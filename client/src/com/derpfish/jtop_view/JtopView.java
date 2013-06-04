package com.derpfish.jtop_view;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.json.JSONException;
import org.json.JSONObject;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GraphViewSeries;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphViewSeries.GraphViewStyle;
import com.jjoe64.graphview.LineGraphView;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.app.Activity;
import android.content.Context;

public class JtopView extends Activity {

	private ScheduledFuture<?> pollTask;
	private Handler mhandler;
	private ScheduledExecutorService scheduler;
	
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
		scheduler = Executors.newScheduledThreadPool(1);
	}
	
	@Override
	public void onResume() {
		super.onResume();

		pollTask = scheduler.scheduleAtFixedRate(new Runnable() {
			@Override
			public void run() {
				try {

					final List<TopData> topDatas = fetchJtop();
					final long initialTs = topDatas.get(0).timestamp;

					final GraphViewData[] gvdCpu = new GraphViewData[topDatas
							.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdCpu[i] = new GraphViewData(topDatas.get(i).timestamp
								- initialTs, topDatas.get(i).data
								.getDouble("cpu_usage"));
					}

					/*
					final GraphViewData[] gvdMem = new GraphViewData[topDatas
							.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdMem[i] = new GraphViewData(topDatas.get(i).timestamp
								- initialTs, topDatas.get(i).data
								.getDouble("mem_usage"));
					}
					*/
					final GraphViewData[] gvdSignal = new GraphViewData[topDatas.size()];
					final GraphViewData[] gvdNoise = new GraphViewData[topDatas.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdSignal[i] = new GraphViewData(
								topDatas.get(i).timestamp- initialTs,
								topDatas.get(i).data.getDouble("iw_signal"));
						gvdNoise[i] = new GraphViewData(
								topDatas.get(i).timestamp- initialTs,
								topDatas.get(i).data.getDouble("iw_noise"));
					}

					/*
					final GraphViewData[] gvdSwap = new GraphViewData[topDatas
							.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdSwap[i] = new GraphViewData(
								topDatas.get(i).timestamp - initialTs, topDatas
										.get(i).data.getDouble("swap_usage"));
					}
					*/
					final GraphViewData[] gvdTcpConns = new GraphViewData[topDatas.size()];
					final GraphViewData[] gvdUdpConns = new GraphViewData[topDatas.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdTcpConns[i] = new GraphViewData(
								topDatas.get(i).timestamp - initialTs, topDatas
										.get(i).data.getDouble("tcp_conns"));
						gvdUdpConns[i] = new GraphViewData(
								topDatas.get(i).timestamp - initialTs, topDatas
										.get(i).data.getDouble("udp_conns"));
					}

					final GraphViewData[] gvdNetIn = new GraphViewData[topDatas
							.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdNetIn[i] = new GraphViewData(
								topDatas.get(i).timestamp - initialTs, topDatas
										.get(i).data.getDouble("net_in"));
					}
					final GraphViewData[] gvdNetOut = new GraphViewData[topDatas
							.size()];
					for (int i = 0; i < topDatas.size(); i++) {
						gvdNetOut[i] = new GraphViewData(
								topDatas.get(i).timestamp - initialTs, topDatas
										.get(i).data.getDouble("net_out"));
					}

					mhandler.post(new Runnable() {
						@Override
						public void run() {
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

				} catch (Exception e) {
					Log.e("JtopView", "Error updating jtop data: " + e.getMessage());
				}
			}

		}, 1L, 1L, TimeUnit.SECONDS);
	}
	
	@Override
	public void onPause() {
		pollTask.cancel(true);
		super.onPause();
	}

	@Override
	public void onDestroy() {
		scheduler.shutdown();
		super.onDestroy();
	}

	private static class TopData {
		private long timestamp;
		private JSONObject data;
	}

	private static List<TopData> fetchJtop() throws IOException, JSONException {
		final byte[] buffer = new byte[4096];
		final ByteArrayOutputStream ostream = new ByteArrayOutputStream();
		final InputStream istream = new URL("http://arkham.arkham.ma:9503/")
				.openStream();
		int nread;
		while ((nread = istream.read(buffer)) > 0) {
			ostream.write(buffer, 0, nread);
		}
		istream.close();
		ostream.close();

		final JSONObject jObj = new JSONObject(
				new String(ostream.toByteArray()));
		final List<TopData> topDatas = new ArrayList<TopData>(jObj.length());
		for (int i = 0; i < jObj.length(); i++) {
			final String timestamp = jObj.names().getString(i);
			final TopData topData = new TopData();
			topData.timestamp = Long.parseLong(timestamp);
			topData.data = jObj.getJSONObject(timestamp);
			topDatas.add(topData);
		}

		Collections.sort(topDatas, new Comparator<TopData>() {
			@Override
			public int compare(TopData a, TopData b) {
				if (a.timestamp < b.timestamp) {
					return -1;
				}
				if (a.timestamp > b.timestamp) {
					return 1;
				}
				return 0;
			}
		});

		return topDatas;
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
