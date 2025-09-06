//
// Copyright (c) 2022-2025 Okkoma Studio.
//

package com.okkomastudio.galaxianmatch;

import java.util.List;
import android.content.Intent;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import io.urho3d.UrhoActivity;

@SuppressLint("SetTextI18n")

public class GalaxianMatch extends UrhoActivity
{
    private static final String TAG = "GalaxianMatch";
    private static final String LOGTAG = "Urho3D";

    // C functions we call
    public static native void PlayTest(int test);
    public static native void AddReward(int rewardid);

    @Override
    protected void onLoadLibrary(List<String> libraryNames)
    {
        libraryNames.add(TAG);
        super.onLoadLibrary(libraryNames);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        Intent launchIntent = getIntent();

        if (launchIntent.getAction().equals("com.google.intent.action.TEST_LOOP"))
        {
            int scenario = launchIntent.getIntExtra("scenario", 0);

            Log.v(LOGTAG, "launchTest scenario="+scenario);

            // Code to handle your game loop here
            PlayTest(scenario);
        }
        else if (launchIntent.getAction().equals("android.intent.action.MAIN"))
        {
            Log.v(LOGTAG, "launchMain");
        }
    }
    
    public void showAds(int time)
    {
        Log.v(LOGTAG, "showAds NoAdsVersion"+ time);
        
        // Need this to Always UnPause the game
        AddReward(0);
    }
}
