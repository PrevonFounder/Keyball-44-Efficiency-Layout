/*
 * SPDX-License-Identifier: MIT
 *
 * Disconnect every non-active host profile once the active profile is connected.
 *
 * zmk_ble_active_profile_changed fires both when the profile is switched and when
 * the active profile's host connects. Acting only when the active profile is
 * connected means advertising is already stopped, so a dropped host cannot
 * reconnect straight away.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static void drop_inactive_profiles(struct k_work *work) {
    if (!zmk_ble_active_profile_is_connected()) {
        return;
    }

    uint8_t active = zmk_ble_active_profile_index();

    for (uint8_t i = 0; i < ZMK_BLE_PROFILE_COUNT; i++) {
        if (i == active) {
            continue;
        }

        int err = zmk_ble_prof_disconnect(i);
        if (err == 0) {
            LOG_DBG("Dropped inactive profile %d", i);
        }
    }
}

K_WORK_DEFINE(drop_inactive_profiles_work, drop_inactive_profiles);

static int ble_exclusive_profile_listener(const zmk_event_t *eh) {
    k_work_submit(&drop_inactive_profiles_work);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ble_exclusive_profile, ble_exclusive_profile_listener);
ZMK_SUBSCRIPTION(ble_exclusive_profile, zmk_ble_active_profile_changed);
