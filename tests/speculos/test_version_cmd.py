def test_version(cmd):
    assert cmd.get_version() == (0, 6, 2)
