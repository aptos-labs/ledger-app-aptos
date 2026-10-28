from pathlib import Path

import tomli
from application_client.boilerplate_command_sender import BoilerplateCommandSender
from application_client.boilerplate_response_unpacker import unpack_get_version_response


# In this test we check the behavior of the device when asked to provide the app version
def test_version(backend):

    with open(Path(__file__).parent.parent.parent / "Cargo.toml", "rb") as f:
        data = tomli.load(f)
    version = tuple(map(int, data["package"]["version"].split(".")))
    # Use the app interface instead of raw interface
    client = BoilerplateCommandSender(backend)
    # Send the GET_VERSION instruction
    version_apdu = client.get_version()
    assert unpack_get_version_response(version_apdu.data) == version
