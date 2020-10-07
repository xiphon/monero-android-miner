// BSD 3-Clause License
//
// Copyright (c) 2020, xiphon <xiphon@protonmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package monero.android.miner;

import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.concurrent.LinkedBlockingQueue;

public class Miner {
    private static Thread miningThread = null;
    private static final LinkedBlockingQueue miningSharesQueue = new LinkedBlockingQueue();
    private static boolean shouldStop = false;

    static {
        System.loadLibrary("monero-android-miner");
    }

    public static native double adjustCpuLoad(double modifier);
    public static native double hashrate();

    public static boolean start(final String host, final int port, final String address, final String worker) {
        if (miningThread != null) {
            return false;
        }
        shouldStop = false;
        miningThread = new Thread(new Runnable() {
            @Override
            public void run() {
                boolean shouldTimeout = false;
                while (!shouldStop) {
                    try {
                        if (shouldTimeout) {
                            Thread.sleep(5 * 1000);
                        } else {
                            shouldTimeout = true;
                        }
                        Stratum stratum = new Stratum(host, port, address, worker, new NewJobCallback() {
                            @Override
                            boolean handler(String id, byte[] blob, byte[] seedHash, long height, byte[] target) {
                                return miningStart(id, blob, seedHash, height, target);
                            }
                        }, new ShareProducer() {
                            @Override
                            JSONObject take() throws InterruptedException {
                                return (JSONObject) miningSharesQueue.take();
                            }
                        });
                        Log.d("Miner", "stratum connected");
                        stratum.run();
                    } catch (Exception e) {
                        Log.e("Miner", "miningThread exception", e);
                    } finally {
                        Log.d("Miner", "stratum disconnected");
                    }
                }
                miningStop();

                miningThread = null;
                miningSharesQueue.clear();

                Log.d("Miner", "stopped");
            }
        });
        miningThread.start();

        Log.d("Miner", "started");
        return true;
    }

    public static void stop() {
        shouldStop = true;
        if (miningThread != null) {
            miningThread.interrupt();
        }
    }

    public static boolean running() {
        return miningThread != null;
    }

    public static void miningCallback(final String jobId, final String hash, final String nonce) {
        try {
            miningSharesQueue.put(new JSONObject(new HashMap<String, Object>() {{
                put("job_id", jobId);
                put("result", hash);
                put("nonce", nonce);
            }}));
        } catch (InterruptedException e) {
        }
    }

    private static native boolean miningStart(String id, byte[] blob, byte[] seedHash, long height, byte[] target);
    private static native void miningStop();
}

abstract class NewJobCallback {
    abstract boolean handler(String id, byte[] blob, byte[] seedHash, long height, byte[] target);
}

abstract class ShareProducer {
    abstract JSONObject take() throws InterruptedException;
}

class Stratum {
    private Socket socket;
    private BufferedReader reader;
    private BufferedWriter writer;
    private String address;
    private String worker;
    private NewJobCallback onNewJob;
    private ShareProducer producer;

    public Stratum(
            String host,
            int port,
            String address,
            String worker,
            NewJobCallback onNewJob,
            ShareProducer producer) throws UnknownHostException, IOException {
        socket = new Socket(InetAddress.getByName(host), port);
        reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
        this.address = address;
        this.worker = worker;
        this.onNewJob = onNewJob;
        this.producer = producer;
    }

    // dARKpRINCE https://stackoverflow.com/a/19119453
    private byte[] hexStringToByteArray(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    private String parseLogin(JSONObject login) {
        try {
            return login.getJSONObject("result").getString("id");
        } catch (Exception e) {
            Log.e("Miner", "failed to parse stratum login response", e);
        }
        return null;
    }

    private JSONObject parseJob(JSONObject json) {
        try {
            String method = json.optString("method");
            if (method.isEmpty()) {
                return json.getJSONObject("result").optJSONObject("job");
            } else if (method.equals("job")) {
                return json.getJSONObject("params");
            }
        } catch (Exception e) {
            Log.e("Miner", "failed to parse stratum response", e);
        }
        return null;
    }

    private JSONObject read() throws IOException, JSONException {
        String line = reader.readLine();
        if (line == null) {
            throw new IOException("connection closed by remote side");
        }

        Log.d("Miner", "stratum <- '" + line + "'");

        return new JSONObject(line);
    }

    private void send(JSONObject request) throws IOException {
        Log.d("Miner", "stratum -> '" + request.toString() + "'");

        writer.write(request.toString());
        writer.newLine();
        writer.flush();
    }

    private JSONObject login() throws IOException, JSONException {
        send(new JSONObject(new HashMap<String, Object>() {{
            put("id", 1);
            put("method", "login");
            put("params", new HashMap<String, Object>() {{
                put("login", address);
                put("rigid", worker);
                put("pass", "x");
                put("agent", "xiphon-java-miner/0.1");
            }});
            put("jsonrpc", "2.0");
        }}));
        return read();
    }

    private void submit(final JSONObject params) throws IOException {
        send(new JSONObject(new HashMap<String, Object>() {{
            put("id", 1);
            put("method", "submit");
            put("params", params);
            put("jsonrpc", "2.0");
        }}));
    }

    private void updateJob(JSONObject job) throws Exception {
        String id = job.getString("job_id");
        String blob = job.getString("blob");
        long height = job.getLong("height");
        String seedHash = job.getString("seed_hash");
        String target = job.getString("target");
        onNewJob.handler(
            id,
            hexStringToByteArray(blob),
            hexStringToByteArray(seedHash),
            height,
            hexStringToByteArray(target));
    }

    public void run() throws Exception, IOException, JSONException {
        final JSONObject login = login();
        final String sessionId = parseLogin(login);

        Thread ioThread = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    JSONObject job = parseJob(login);
                    if (job == null) {
                        throw new Exception("failed to parse initial job");
                    }
                    while (true) {
                        if (job != null) {
                            updateJob(job);
                        }
                        job = parseJob(read());
                    }
                } catch (SocketException e) {
                } catch (Exception e) {
                    Log.d("Miner", "stratum error: " + e.toString());
                }
            }
        });
        ioThread.start();

        try {
            JSONObject share;
            while (true) {
                share = producer.take();
                share.put("id", sessionId);
                submit(share);
            }
        } catch (InterruptedException e) {
        }

        try {
            socket.close();
        } finally {
            if (ioThread.isAlive()) {
                ioThread.interrupt();
                ioThread.join();
            }
        }
    }
}
