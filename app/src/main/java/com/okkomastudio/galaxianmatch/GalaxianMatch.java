//
// Copyright (c) 2022-2024 Okkoma Studio.
//

package com.okkomastudio.galaxianmatch;

import com.okkomastudio.galaxianmatch.AdDialogFragment.AdDialogInteractionListener;

import java.util.List;
import android.content.Intent;
import android.content.Context;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.Log;
//import android.widget.Toast;
import android.net.ConnectivityManager;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.OnUserEarnedRewardListener;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.rewarded.RewardItem;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAd;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAdLoadCallback;

import io.urho3d.UrhoActivity;

@SuppressLint("SetTextI18n")

public class GalaxianMatch extends UrhoActivity
{
    private static final String TAG = "GalaxianMatch";
    private static final String LOGTAG = "Urho3D";
    private static final String AD_UNIT_ID = "ca-app-pub-5481246747854549/4348729882";
    public static boolean showAdsSuccess = false;

    // C functions we call
    public static native void PlayTest(int test);
    public static native void AddReward(int rewardid);


    private RewardedInterstitialAd rewardedInterstitialAd;
    boolean isMobileAdsInitialized = false;
    boolean isLoadingAds = false;

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

        isMobileAdsInitialized = initializeMobileAds();

        Intent launchIntent = getIntent();
        if (launchIntent.getAction().equals("com.google.intent.action.TEST_LOOP"))
        {
            int scenario = launchIntent.getIntExtra("scenario", 1);

            Log.v(LOGTAG, "launchTest scenario="+scenario);

            // Code to handle your game loop here
            PlayTest(scenario);
        }
    }
    
    private boolean isNetworkAvailable() 
    {
        ConnectivityManager connManager = (ConnectivityManager) getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        return ((connManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE) != null && connManager
                .getNetworkInfo(ConnectivityManager.TYPE_MOBILE).isConnected())
                || (connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI) != null && connManager
                .getNetworkInfo(ConnectivityManager.TYPE_WIFI)
                .isConnected()));
    }
    
    private boolean initializeMobileAds()
    {
        if (!isMobileAdsInitialized && isNetworkAvailable())
        {
            MobileAds.initialize(this,
                new OnInitializationCompleteListener()
                {
                    @Override
                    public void onInitializationComplete(InitializationStatus initializationStatus)
                    {
                        loadRewardedInterstitialAd();
                    }
                }
            );
            isMobileAdsInitialized = true;
        }
        return isMobileAdsInitialized;
    }
    
    private void loadRewardedInterstitialAd()
    {
        Log.v(LOGTAG, "loadRewardedInterstitialAd ");

        if (rewardedInterstitialAd == null)
        {
            isLoadingAds = true;

            Log.v(LOGTAG, "loadRewardedInterstitialAd - Create RewardAd");

            AdRequest adRequest = new AdRequest.Builder().build();
            rewardedInterstitialAd.load(
                this,
                AD_UNIT_ID,
                adRequest,
                new RewardedInterstitialAdLoadCallback()
                {
                    @Override
                    public void onAdLoaded(RewardedInterstitialAd ad)
                    {
                        Log.d(LOGTAG, "onAdLoaded");

                        rewardedInterstitialAd = ad;
                        isLoadingAds = false;
                        //Toast.makeText(GalaxianMatch.this, "onAdLoaded", Toast.LENGTH_SHORT).show();
                    }

                    @Override
                    public void onAdFailedToLoad(LoadAdError loadAdError)
                    {
                        Log.d(LOGTAG, "onAdFailedToLoad: " + loadAdError.getMessage());

                        // Handle the error.
                        rewardedInterstitialAd = null;
                        isLoadingAds = false;
                        //Toast.makeText(GalaxianMatch.this, "onAdFailedToLoad", Toast.LENGTH_SHORT).show();
                    }
                }
            );
        }
    }

    // JNI Call for Ads
    public boolean showAds(int time)
    {
        Log.v(LOGTAG, "showAds "+ time);
        showAdsSuccess = false;
        
        if (!isNetworkAvailable())
        {
            Log.w(LOGTAG, "no network connection !");
            return false;
        }
        
        if (!isMobileAdsInitialized)
        {
            Log.w(LOGTAG, "mobileAds not intialiazed!");
            return false;         
        }
        
        if (rewardedInterstitialAd == null)
            loadRewardedInterstitialAd();
        
        if (rewardedInterstitialAd == null)
            return false;

        if (time > 0)
        {
            CountDownTimer countDownTimer = new CountDownTimer(time * 1000, 50)
            {
                @Override
                public void onTick(long millisUnitFinished)
                {

                }
                @Override
                public void onFinish()
                {
                    showAdsSuccess = introduceVideoAd(1, "Star");
                }
            };

            countDownTimer.start();
        }
        else
        {
            showAdsSuccess = introduceVideoAd(1, "Star");
        }
        return showAdsSuccess;
    }
    // JNI Call for Test
    public void finishTest()
    {
        Log.v(LOGTAG, "finishTest");
        finish();
    }
    
    private boolean introduceVideoAd(int rewardAmount, String rewardType)
    {
        Log.v(LOGTAG, "introduceVideoAd ");
        if (rewardedInterstitialAd == null)
        {
            Log.w(LOGTAG, "The rewarded ad wasn't ready yet. load now and return");
            loadRewardedInterstitialAd();
            return false;
        }

        AdDialogFragment dialog = AdDialogFragment.newInstance(rewardAmount, rewardType);
        dialog.setAdDialogInteractionListener(
            new AdDialogInteractionListener()
            {
                @Override
                public void onShowAd()
                {
                    Log.d(TAG, "The rewarded interstitial ad is starting.");

                    showRewardedVideo();
                }

                @Override
                public void onCancelAd()
                {
                    Log.d(TAG, "The rewarded interstitial ad was skipped before it starts.");
                    // Need this to Always UnPause the game
                    AddReward(0);
                }
            }
        );
        
        dialog.show(getSupportFragmentManager(), "AdDialogFragment");
        //dialog.show(getParentFragmentManager(), "AdDialogFragment");
        return true;
    }

    private void showRewardedVideo()
    {
        if (rewardedInterstitialAd == null)
        {
            Log.w(LOGTAG, "The rewarded ad wasn't ready yet.");
            return;
        }

        Log.v(LOGTAG, "showRewardedVideo.");

        rewardedInterstitialAd.setFullScreenContentCallback(
            new FullScreenContentCallback()
            {
                @Override
                public void onAdShowedFullScreenContent()
                {
                    // Called when ad is shown.
                    Log.v(LOGTAG, "onAdShowedFullScreenContent");
                    //Toast.makeText(GalaxianMatch.this, "onAdShowedFullScreenContent", Toast.LENGTH_SHORT).show();
                }

                @Override
                public void onAdFailedToShowFullScreenContent(AdError adError)
                {
                    // Called when ad fails to show.
                    Log.d(LOGTAG, "onAdFailedToShowFullScreenContent");
                    //Toast.makeText(GalaxianMatch.this, "onAdFailedToShowFullScreenContent", Toast.LENGTH_SHORT).show();

                    // Preload the next rewarded interstitial ad.
                    rewardedInterstitialAd = null;
                    loadRewardedInterstitialAd();
                }

                @Override
                public void onAdDismissedFullScreenContent()
                {
                    // Called when ad is dismissed.
                    Log.v(LOGTAG, "onAdDismissedFullScreenContent");
                    //Toast.makeText(GalaxianMatch.this, "onAdDismissedFullScreenContent", Toast.LENGTH_SHORT).show();

                    // Preload the next rewarded interstitial ad.
                    rewardedInterstitialAd = null;
                    loadRewardedInterstitialAd();

                    // Need this to Always UnPause the game
                    AddReward(0);
                }
            }
        );

        rewardedInterstitialAd.show(
            this,
            new OnUserEarnedRewardListener()
            {
                @Override
                public void onUserEarnedReward(@NonNull RewardItem rewardItem)
                {
                    // Handle the reward.
                    Log.v(TAG, "The user earned the reward.");
                    AddReward(1);
                }
            }
        );
    }   
}
