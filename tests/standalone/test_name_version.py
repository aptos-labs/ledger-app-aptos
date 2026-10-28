from pathlib import Path

import tomli
from application_client.boilerplate_command_sender import BoilerplateCommandSender
from application_client.boilerplate_response_unpacker import unpack_get_app_and_version_response


# BOLOS, rather than the app, reports the installed application's name and version.
def test_get_app_and_version(backend):
    with open(Path(__file__).parent.parent.parent / "Cargo.toml", "rb") as package_file:
        version = tomli.load(package_file)["package"]["version"]

    client = BoilerplateCommandSender(backend)
    app_and_version_apdu = client.get_app_and_version()
    app_name, reported_version = unpack_get_app_and_version_response(app_and_version_apdu.data)

    assert app_name == "Aptos"
    assert reported_version == version
