from ragger.navigator import NavIns, NavInsID


# In this test we check the behavior of the device main menu
def test_app_mainmenu(device, firmware, navigator, test_name, default_screenshot_path):
    # Navigate in the main menu
    if device.is_nano:
        instructions = [
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
        ]
    else:
        num_info_pages = 2 if firmware.device == "flex" else 1
        instructions = [
            NavInsID.USE_CASE_HOME_SETTINGS,
            NavIns(NavInsID.TOUCH, (200, 113)),
            NavIns(NavInsID.TOUCH, (200, 113)),
            *([NavInsID.USE_CASE_SETTINGS_NEXT] * num_info_pages),
            NavInsID.USE_CASE_SETTINGS_PREVIOUS,
            NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT,
        ]
    navigator.navigate_and_compare(
        default_screenshot_path,
        test_name,
        instructions,
        screen_change_before_first_instruction=False,
    )
