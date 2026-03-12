suite('EXRTC');

platform('bsom', 'b5som', 'esomx', 'msom', 'tracker');
timeout(5 * 60 * 1000);

let device = null;

before(function() {
    device = this.particle.devices[0];
});

test('EXRTC_00_initialize_suite_state', async function() {
});

test('EXRTC_01_time_sources_are_in_sync_on_boot', async function() {
});

test('EXRTC_02_id_is_non_empty', async function() {
});

test('EXRTC_03_dump_system_cache', async function() {
    const msg = device.mailBox.pop();
    console.log('System cache dump:\n%s', JSON.stringify(JSON.parse(msg.d), null, 2));
});

test('EXRTC_04_enable_same_config_does_not_reset_or_lose_time', async function() {
});

test('EXRTC_05_set_config_same_config_does_not_reset_or_lose_time', async function() {
});

test('EXRTC_06_reconfigure_and_rebind_preserves_time_sync', async function() {
});

test('EXRTC_07_update_configuration', async function() {
});

test('EXRTC_08_power_off_should_fail_without_internal_fallback', async function() {
});

test('EXRTC_09_power_off_should_succeed_1', async function() {
});

test('EXRTC_09_power_off_should_succeed_2', async function() {
});

test('EXRTC_10_prepare_legacy_cache_calibration_and_reset', async function() {
});

test('EXRTC_11_legacy_cache_calibration_is_used_when_no_override_is_set', async function() {
});

test('EXRTC_12_config_calibration_overrides_legacy_cache_value', async function() {
});

test('EXRTC_13_prepare_new_cache_calibration_and_reset', async function() {
});

test('EXRTC_14_new_cache_calibration_is_used_when_no_override_is_set', async function() {
});

test('EXRTC_15_config_calibration_overrides_new_cache_value', async function() {
});

test('EXRTC_16_prepare_default_calibration_case_and_reset', async function() {
});

test('EXRTC_17_default_calibration_is_used_when_no_cache_or_override_exists', async function() {
});

test('EXRTC_99_restore_default_configuration', async function() {
});
