CONFIG_FEATURES:=CONFIG_TEST_CFLAGS

# This sets the execution modes that will be supported by the build (vm, native or assisted)
CONFIG_EXECUTION_MODES:=assisted

# Set the lib directories to exclude in the build (if unset all libraries are included)
CONFIG_DIRS_FILTER:= a2dp aghfp anc audio audio_plugin_common audio_plugin_if avrcp bdaddr byte_utils \
                     gain_utils connection csr_a2dp_decoder_common_plugin csr_a2dp_encoder_common_plugin \
                     csr_ag_audio_plugin csr_common_example_plugin csr_cvc_common_plugin csr_dut_audio_plugin \
                     csr_i2s_audio_plugin csr_multi_channel_plugin csr_subwoofer_plugin csr_voice_prompts_plugin debongle display \
                     display_example_plugin display_plugin_cns10010 display_plugin_midas display_plugin_if fm_plugin_if fm_rx_api fm_rx_plugin gaia gatt \
                     gatt_apple_notification_client gatt_battery_client gatt_battery_server gatt_client gatt_device_info_client \
                     gatt_device_info_server gatt_gap_server gatt_heart_rate_client gatt_heart_rate_server gatt_hid_client gatt_imm_alert_client \
                     gatt_imm_alert_server gatt_link_loss_server gatt_manager gatt_scan_params_client gatt_server gatt_transmit_power_server hfp \
                     iap2 kalimba_standard_messages leds_flash leds_manager leds_manager_if leds_rom library mapc md5 obex obex_parse pbapc \
                     pblock pio_common pio_monitor power power_onchip power_onchip_adapter print region sdp_parse service spp_common sppc spps swat upgrade usb_device_class \
                      vmal wbs firmware_spies message_mock message_spy vmtypes transform_mock operator_mock operator_list_spy \
                     stream_mock sink_mock source_mock stream_spy i2c_spy ps_transform_test sink operator_spy \
                     adc_mock boot_mock charger_mock codec_mock feature_mock file_mock kalimba_mock led_mock loader_mock mic_bias_mock panic_mock \
                     partition_mock pio_mock ps_mock psu_mock test_mock vm_mock
