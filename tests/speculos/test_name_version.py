def test_get_app_and_version(cmd):
    app_name, version = cmd.get_app_and_version()

    assert app_name == "Aptos"
    assert version == "0.4.17"
