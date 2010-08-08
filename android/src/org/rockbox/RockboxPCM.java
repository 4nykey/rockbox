/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

package org.rockbox;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;

public class RockboxPCM extends AudioTrack 
{
	byte[] raw_data;
	private int buf_len;
	private PCMListener l;
	private HandlerThread ht;
	private Handler h = null;

	private void LOG(CharSequence text)
	{
		Log.d("Rockbox", (String) text);
	}

	public RockboxPCM()
	{
		super(AudioManager.STREAM_MUSIC, 
			    44100,
			    /* should be CHANNEL_OUT_STEREO in 2.0 and above */
			    AudioFormat.CHANNEL_CONFIGURATION_STEREO,
			    AudioFormat.ENCODING_PCM_16BIT,
			    24<<10,
			    AudioTrack.MODE_STREAM);
		ht = new HandlerThread("audio thread", Process.THREAD_PRIORITY_URGENT_AUDIO);
		ht.start();
		buf_len = 24<<10; /* in bytes */

	    raw_data = new byte[buf_len]; /* in shorts */
	    for(int i = 0; i < raw_data.length; i++) raw_data[i] = (byte)0;
	    l = new PCMListener(buf_len);
	}    

	int bytes2frames(int bytes) {
		/* 1 sample is 2 bytes, 2 samples are 1 frame */
		return (bytes/4);
	}
	
	int frames2bytes(int frames) {
		/* 1 frame is 2 samples, 1 sample is 2 bytes */
		return (frames*4);
	}

    @SuppressWarnings("unused")
	private void play_pause(boolean pause) {
    	if (pause)
    	{
    		pause();
    	}
    	else
    	{
    		if (getPlayState() == AudioTrack.PLAYSTATE_STOPPED)
    		{
    	        RockboxService.startForeground();
        	    if (getState() == AudioTrack.STATE_INITIALIZED)
        	    {
        	    	if (h == null)
        	    		h = new Handler(ht.getLooper());
        	    	if (setNotificationMarkerPosition(bytes2frames(buf_len)/4) != AudioTrack.SUCCESS)
        	    		LOG("setNotificationMarkerPosition Error");
        	    	else
        	    		setPlaybackPositionUpdateListener(l, h);
        	    }
        	    /* need to fill with silence before starting playback */
        		write(raw_data, frames2bytes(getPlaybackHeadPosition()), raw_data.length);
    		}
    		play();
    	}
    }
    
    @Override
    public void stop() throws IllegalStateException 
    {
    	try {
    		super.stop();
    	} catch (IllegalStateException e) {
    		throw new IllegalStateException(e);
    	}
    	RockboxService.stopForeground();
    }

    @SuppressWarnings("unused")
    private void set_volume(int volume)
    {
    	/* volume comes from 0..-990 from Rockbox */
    	/* TODO volume is in dB, but this code acts as if it were in %, convert? */
    	float fvolume;
    	/* special case min and max volume to not suffer from floating point accuracy */
    	if (volume == 0)
    		fvolume = 1.0f;
    	else if (volume == -990)
    		fvolume = 0.0f;
    	else
    		fvolume = (volume + 990)/990.0f;
    	setStereoVolume(fvolume, fvolume);
    }

    public native void pcmSamplesToByteArray(byte[] dest);
   
    private class PCMListener implements OnPlaybackPositionUpdateListener {
        int max_len;
        int refill_mark;
        byte[] buf;
		public PCMListener(int len) {
            max_len = len;
            /* refill to 100% when reached the 25% */
            buf = new byte[max_len*3/4];
            refill_mark = max_len - buf.length;
		}
		@Override
		public void onMarkerReached(AudioTrack track) {
			// push new data to the hardware
			RockboxPCM pcm = (RockboxPCM)track;
			int result = -1;
			pcm.pcmSamplesToByteArray(buf);
			result = track.write(buf, 0, buf.length);
			if (result >= 0)
			{
				switch(track.getPlayState())
				{
				case AudioTrack.PLAYSTATE_PLAYING:
				case AudioTrack.PLAYSTATE_PAUSED:
					/* refill at 25% no matter of how many bytes we've written */
					if (setNotificationMarkerPosition(bytes2frames(refill_mark)) != AudioTrack.SUCCESS)
						LOG("Error in onMarkerReached: Could not set notification marker");
					else /* recharge */
						setPlaybackPositionUpdateListener(this, h);
					break;
				case AudioTrack.PLAYSTATE_STOPPED:
					LOG("State STOPPED");
					break;
				}
			}
			else
			{
				LOG("Error in onMarkerReached (result="+result+")");
				stop();
			}
		}

		@Override
		public void onPeriodicNotification(AudioTrack track) {
			// TODO Auto-generated method stub
			
		}
	}
}
