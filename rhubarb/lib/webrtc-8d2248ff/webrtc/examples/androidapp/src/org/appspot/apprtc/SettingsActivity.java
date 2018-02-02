/*
 *  Copyright 2014 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.appspot.apprtc;

import android.app.Activity;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;

import org.webrtc.voiceengine.WebRtcAudioUtils;

/**
 * Settings activity for AppRTC.
 */
public class SettingsActivity extends Activity
    implements OnSharedPreferenceChangeListener{
  private SettingsFragment settingsFragment;
  private String keyprefVideoCall;
  private String keyprefResolution;
  private String keyprefFps;
  private String keyprefCaptureQualitySlider;
  private String keyprefStartVideoBitrateType;
  private String keyprefStartVideoBitrateValue;
  private String keyPrefVideoCodec;
  private String keyprefHwCodec;
  private String keyprefCaptureToTexture;

  private String keyprefStartAudioBitrateType;
  private String keyprefStartAudioBitrateValue;
  private String keyPrefAudioCodec;
  private String keyprefNoAudioProcessing;
  private String keyprefAecDump;
  private String keyprefOpenSLES;
  private String keyprefDisableBuiltInAEC;

  private String keyPrefRoomServerUrl;
  private String keyPrefDisplayHud;
  private String keyPrefTracing;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    keyprefVideoCall = getString(R.string.pref_videocall_key);
    keyprefResolution = getString(R.string.pref_resolution_key);
    keyprefFps = getString(R.string.pref_fps_key);
    keyprefCaptureQualitySlider = getString(R.string.pref_capturequalityslider_key);
    keyprefStartVideoBitrateType = getString(R.string.pref_startvideobitrate_key);
    keyprefStartVideoBitrateValue = getString(R.string.pref_startvideobitratevalue_key);
    keyPrefVideoCodec = getString(R.string.pref_videocodec_key);
    keyprefHwCodec = getString(R.string.pref_hwcodec_key);
    keyprefCaptureToTexture = getString(R.string.pref_capturetotexture_key);

    keyprefStartAudioBitrateType = getString(R.string.pref_startaudiobitrate_key);
    keyprefStartAudioBitrateValue = getString(R.string.pref_startaudiobitratevalue_key);
    keyPrefAudioCodec = getString(R.string.pref_audiocodec_key);
    keyprefNoAudioProcessing = getString(R.string.pref_noaudioprocessing_key);
    keyprefAecDump = getString(R.string.pref_aecdump_key);
    keyprefOpenSLES = getString(R.string.pref_opensles_key);
    keyprefDisableBuiltInAEC = getString(R.string.pref_disable_built_in_aec_key);

    keyPrefRoomServerUrl = getString(R.string.pref_room_server_url_key);
    keyPrefDisplayHud = getString(R.string.pref_displayhud_key);
    keyPrefTracing = getString(R.string.pref_tracing_key);

    // Display the fragment as the main content.
    settingsFragment = new SettingsFragment();
    getFragmentManager().beginTransaction()
        .replace(android.R.id.content, settingsFragment)
        .commit();
  }

  @Override
  protected void onResume() {
    super.onResume();
    // Set summary to be the user-description for the selected value
    SharedPreferences sharedPreferences =
        settingsFragment.getPreferenceScreen().getSharedPreferences();
    sharedPreferences.registerOnSharedPreferenceChangeListener(this);
    updateSummaryB(sharedPreferences, keyprefVideoCall);
    updateSummary(sharedPreferences, keyprefResolution);
    updateSummary(sharedPreferences, keyprefFps);
    updateSummaryB(sharedPreferences, keyprefCaptureQualitySlider);
    updateSummary(sharedPreferences, keyprefStartVideoBitrateType);
    updateSummaryBitrate(sharedPreferences, keyprefStartVideoBitrateValue);
    setVideoBitrateEnable(sharedPreferences);
    updateSummary(sharedPreferences, keyPrefVideoCodec);
    updateSummaryB(sharedPreferences, keyprefHwCodec);
    updateSummaryB(sharedPreferences, keyprefCaptureToTexture);

    updateSummary(sharedPreferences, keyprefStartAudioBitrateType);
    updateSummaryBitrate(sharedPreferences, keyprefStartAudioBitrateValue);
    setAudioBitrateEnable(sharedPreferences);
    updateSummary(sharedPreferences, keyPrefAudioCodec);
    updateSummaryB(sharedPreferences, keyprefNoAudioProcessing);
    updateSummaryB(sharedPreferences, keyprefAecDump);
    updateSummaryB(sharedPreferences, keyprefOpenSLES);
    updateSummaryB(sharedPreferences, keyprefDisableBuiltInAEC);

    updateSummary(sharedPreferences, keyPrefRoomServerUrl);
    updateSummaryB(sharedPreferences, keyPrefDisplayHud);
    updateSummaryB(sharedPreferences, keyPrefTracing);

    // Disable forcing WebRTC based AEC so it won't affect our value.
    // Otherwise, if it was enabled, isAcousticEchoCancelerSupported would always return false.
    WebRtcAudioUtils.setWebRtcBasedAcousticEchoCanceler(false);
    if (!WebRtcAudioUtils.isAcousticEchoCancelerSupported()) {
      Preference disableBuiltInAECPreference =
          settingsFragment.findPreference(keyprefDisableBuiltInAEC);


      disableBuiltInAECPreference.setSummary(getString(R.string.pref_built_in_aec_not_available));
      disableBuiltInAECPreference.setEnabled(false);
    }
  }

  @Override
  protected void onPause() {
    super.onPause();
    SharedPreferences sharedPreferences =
        settingsFragment.getPreferenceScreen().getSharedPreferences();
    sharedPreferences.unregisterOnSharedPreferenceChangeListener(this);
  }

  @Override
  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
      String key) {
    if (key.equals(keyprefResolution)
        || key.equals(keyprefFps)
        || key.equals(keyprefStartVideoBitrateType)
        || key.equals(keyPrefVideoCodec)
        || key.equals(keyprefStartAudioBitrateType)
        || key.equals(keyPrefAudioCodec)
        || key.equals(keyPrefRoomServerUrl)) {
      updateSummary(sharedPreferences, key);
    } else if (key.equals(keyprefStartVideoBitrateValue)
        || key.equals(keyprefStartAudioBitrateValue)) {
      updateSummaryBitrate(sharedPreferences, key);
    } else if (key.equals(keyprefVideoCall)
        || key.equals(keyPrefTracing)
        || key.equals(keyprefCaptureQualitySlider)
        || key.equals(keyprefHwCodec)
        || key.equals(keyprefCaptureToTexture)
        || key.equals(keyprefNoAudioProcessing)
        || key.equals(keyprefAecDump)
        || key.equals(keyprefOpenSLES)
        || key.equals(keyprefDisableBuiltInAEC)
        || key.equals(keyPrefDisplayHud)) {
      updateSummaryB(sharedPreferences, key);
    }
    if (key.equals(keyprefStartVideoBitrateType)) {
      setVideoBitrateEnable(sharedPreferences);
    }
    if (key.equals(keyprefStartAudioBitrateType)) {
      setAudioBitrateEnable(sharedPreferences);
    }
  }

  private void updateSummary(SharedPreferences sharedPreferences, String key) {
    Preference updatedPref = settingsFragment.findPreference(key);
    // Set summary to be the user-description for the selected value
    updatedPref.setSummary(sharedPreferences.getString(key, ""));
  }

  private void updateSummaryBitrate(
      SharedPreferences sharedPreferences, String key) {
    Preference updatedPref = settingsFragment.findPreference(key);
    updatedPref.setSummary(sharedPreferences.getString(key, "") + " kbps");
  }

  private void updateSummaryB(SharedPreferences sharedPreferences, String key) {
    Preference updatedPref = settingsFragment.findPreference(key);
    updatedPref.setSummary(sharedPreferences.getBoolean(key, true)
        ? getString(R.string.pref_value_enabled)
        : getString(R.string.pref_value_disabled));
  }

  private void setVideoBitrateEnable(SharedPreferences sharedPreferences) {
    Preference bitratePreferenceValue =
        settingsFragment.findPreference(keyprefStartVideoBitrateValue);
    String bitrateTypeDefault = getString(R.string.pref_startvideobitrate_default);
    String bitrateType = sharedPreferences.getString(
        keyprefStartVideoBitrateType, bitrateTypeDefault);
    if (bitrateType.equals(bitrateTypeDefault)) {
      bitratePreferenceValue.setEnabled(false);
    } else {
      bitratePreferenceValue.setEnabled(true);
    }
  }

  private void setAudioBitrateEnable(SharedPreferences sharedPreferences) {
    Preference bitratePreferenceValue =
        settingsFragment.findPreference(keyprefStartAudioBitrateValue);
    String bitrateTypeDefault = getString(R.string.pref_startaudiobitrate_default);
    String bitrateType = sharedPreferences.getString(
        keyprefStartAudioBitrateType, bitrateTypeDefault);
    if (bitrateType.equals(bitrateTypeDefault)) {
      bitratePreferenceValue.setEnabled(false);
    } else {
      bitratePreferenceValue.setEnabled(true);
    }
  }
}
