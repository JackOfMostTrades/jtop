package com.derpfish.jtop_view;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.text.DecimalFormat;
import java.util.LinkedList;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GraphViewSeries;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphViewSeries.GraphViewStyle;
import com.jjoe64.graphview.LineGraphView;

import android.os.Bundle;
import android.os.Handler;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.app.Activity;
import android.content.Context;

public class JtopView extends Activity {

	private DataProviderMain dataProviderMain;
	private Thread bgThread;
	private Handler mhandler;
	
	private final DecimalFormat dfNetUsage = new DecimalFormat("0.0");
	private final DecimalFormat dfPercent = new DecimalFormat(".00");
	
	private GraphViewSeries cpuUsageSeries,
		//memUsageSeries,
	    iwSignalSeries,
	    iwNoiseSeries,
		//swapUsageSeries,
		tcpConnsSeries,
		udpConnsSeries,
		netInSeries,
		netOutSeries; 
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		// init example series data
		cpuUsageSeries = new GraphViewSeries(
				new GraphViewData[] { new GraphViewData(0, 0) });
		//memUsageSeries = new GraphViewSeries(
		//		new GraphViewData[] { new GraphViewData(0, 0) });
		iwSignalSeries = new GraphViewSeries(
				new GraphViewData[] { new GraphViewData(0, 0) });
		iwNoiseSeries = new GraphViewSeries(null,
				new GraphViewStyle(0xffcc7700, 3),
				new GraphViewData[] { new GraphViewData(0, 0) });
		//swapUsageSeries = new GraphViewSeries(
		//		new GraphViewData[] { new GraphViewData(0, 0) });
		tcpConnsSeries = new GraphViewSeries(
				new GraphViewData[] { new GraphViewData(0, 0) });
		udpConnsSeries = new GraphViewSeries(null,
				new GraphViewStyle(0xffcc7700, 3),
				new GraphViewData[] { new GraphViewData(0, 0) });
		
		netInSeries = new GraphViewSeries(
				new GraphViewData[] { new GraphViewData(0, 0) });
		netOutSeries = new GraphViewSeries(null,
				new GraphViewStyle(0xffcc7700, 3),
				new GraphViewData[] { new GraphViewData(0, 0) });

		final GraphView[] views = new GraphView[4];
		views[0] = new DecimalFormatLineGraphView(this, "CPU", null, dfPercent);
		//views[1] = new DecimalFormatLineGraphView(this, "Memory", null, dfPercent);
		views[1] = new LineGraphView(this, "Signal/Noise");
		//views[2] = new DecimalFormatLineGraphView(this, "Swap", null, dfPercent);
		views[2] = new LineGraphView(this, "Connections");
		views[3] = new LineGraphView(this, "Network") {
			@Override
			protected String formatLabel(double value, boolean isValueX) {
				if (!isValueX) {
					if (value >= 1024.0*1024.0) {
						return dfNetUsage.format(value / 1024.0*1024.0) + "M";
					} else if (value >= 1024.0) {
						return dfNetUsage.format(value / 1024.0) + "k";
					}
				}
				// Otherwise return the usual formatting
				return super.formatLabel(value, isValueX);
			}
		};

		views[0].addSeries(cpuUsageSeries);
		//views[1].addSeries(memUsageSeries);
		views[1].addSeries(iwSignalSeries);
		views[1].addSeries(iwNoiseSeries);
		//views[2].addSeries(swapUsageSeries);
		views[2].addSeries(tcpConnsSeries);
		views[2].addSeries(udpConnsSeries);
		views[3].addSeries(netInSeries);
		views[3].addSeries(netOutSeries);

		setContentView(R.layout.main_view);
		((LinearLayout) this.findViewById(R.id.graph1)).addView(views[0]);
		((LinearLayout) this.findViewById(R.id.graph2)).addView(views[3]);
		((LinearLayout) this.findViewById(R.id.graph3)).addView(views[1]);
		((LinearLayout) this.findViewById(R.id.graph4)).addView(views[2]);

		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, 
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
		
		// Setup updates
		mhandler = new Handler();
		
	}
	
	@Override
	public void onResume() {
		super.onResume();
		dataProviderMain = new DataProviderMain();
		bgThread = new Thread(dataProviderMain);
		bgThread.start();
	}
	
	@Override
	public void onPause() {
		dataProviderMain.shouldShutdown = true;
		bgThread.interrupt();
		try {
			bgThread.join();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		super.onPause();
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
	}

	private List<JSONObject> dataHistory = new LinkedList<JSONObject>();
	
	private class DataProviderMain implements Runnable {

		public boolean shouldShutdown = false;
		
		@Override
		public void run() {
			final byte[] buffer = new byte[4096];
			while (!shouldShutdown) {
				Socket socket = null;
				InputStream istream = null;
				
				try {
					socket = new Socket("arkham.arkham.ma", 9503);
					istream = socket.getInputStream();
					ByteArrayOutputStream baos = new ByteArrayOutputStream();
					
					while (!shouldShutdown) {
						int nread = istream.read(buffer);
						if (nread <= 0) {
							break;
						}
						
						boolean triggerRedraw = false;
						int startIndex = 0;
						while (startIndex < nread) {
							int nullByte;
							for (nullByte = startIndex; nullByte < nread; nullByte++) {
								if (buffer[nullByte] == '\0') {
									break;
								}
							}
							baos.write(buffer, startIndex, nullByte);
							
							// We have a complete packet, so handle it
							if (nullByte < nread) {
								final String json = new String(baos.toByteArray());
								
								final JSONObject jsonObj = new JSONObject(json);
								synchronized(dataHistory) {
									dataHistory.add(jsonObj);
									while (dataHistory.size() > 30) dataHistory.remove(0);
								}
								triggerRedraw = true;
								
								// Create a new buffer for next datagram
								baos.close();
								baos = new ByteArrayOutputStream();
							}
							
							startIndex = nullByte+1;
						}
						
						if (triggerRedraw) {
							mhandler.post(new Runnable() {
								@Override
								public void run() {
									GraphViewData[] gvdCpu, gvdSignal, gvdNoise, gvdTcpConns, gvdUdpConns, gvdNetIn, gvdNetOut;
									synchronized(dataHistory) {
										gvdCpu = new GraphViewData[dataHistory.size()];
										gvdSignal = new GraphViewData[dataHistory.size()];
										gvdNoise = new GraphViewData[dataHistory.size()];
										gvdTcpConns = new GraphViewData[dataHistory.size()];
										gvdUdpConns = new GraphViewData[dataHistory.size()];
										gvdNetIn = new GraphViewData[dataHistory.size()];
										gvdNetOut = new GraphViewData[dataHistory.size()];
										try {
											for (int i = 0; i < dataHistory.size(); i++) {
												gvdCpu[i] = new GraphViewData(i, dataHistory.get(i).getDouble("cpu_usage"));
												gvdSignal[i] = new GraphViewData(i, dataHistory.get(i).getDouble("iw_signal"));
												gvdNoise[i] = new GraphViewData(i, dataHistory.get(i).getDouble("iw_noise"));
												gvdTcpConns[i] = new GraphViewData(i, dataHistory.get(i).getDouble("tcp_conns"));
												gvdUdpConns[i] = new GraphViewData(i, dataHistory.get(i).getDouble("udp_conns"));
												gvdNetIn[i] = new GraphViewData(i, dataHistory.get(i).getDouble("net_in"));
												gvdNetOut[i] = new GraphViewData(i, dataHistory.get(i).getDouble("net_out"));
											}
										} catch (JSONException e) {
											e.printStackTrace();
										}
									}
									
									cpuUsageSeries.resetData(gvdCpu);
									//memUsageSeries.resetData(gvdMem);
									iwSignalSeries.resetData(gvdSignal);
									iwNoiseSeries.resetData(gvdNoise);
									//swapUsageSeries.resetData(gvdSwap);
									tcpConnsSeries.resetData(gvdTcpConns);
									udpConnsSeries.resetData(gvdUdpConns);
									netInSeries.resetData(gvdNetIn);
									netOutSeries.resetData(gvdNetOut);
								}
							});
						}
					}
				}
				catch (UnknownHostException e) {}
				catch (IOException e) {}
				catch (JSONException e) {}
				
				try {
					if (socket != null) socket.close();
				} catch (IOException e) {}
				try {
					if (istream != null) istream.close();
				} catch (IOException e) {}
				
				try {
					Thread.sleep(1000L);
				} catch(InterruptedException e) {}
			}
		}
		
	}
	
	private static class DecimalFormatLineGraphView extends LineGraphView {
		private final DecimalFormat dfX;
		private final DecimalFormat dfY;
		
		public DecimalFormatLineGraphView(Context context, String title,
				final DecimalFormat dfX, final DecimalFormat dfY) {
			super(context, title);
			this.dfX = dfX;
			this.dfY = dfY;
		}

		@Override
		protected String formatLabel(double value, boolean isValueX) {
			if (isValueX && dfX != null) {
				return dfX.format(value);
			}
			if (!isValueX && dfY != null) {
				return dfY.format(value);
			}
			// Return default format otherwise
			return super.formatLabel(value, isValueX);
		}
	}
}
