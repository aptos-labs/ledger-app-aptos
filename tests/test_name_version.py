from application_client.aptos_command_sender import AptosCommandSender
from application_client.aptos_response_unpacker import unpack_get_app_and_version_response
from utils import verify_version


# Test a specific APDU asking BOLOS (and not the app) the name and version of the current app
def test_get_app_and_version(backend):
    # Use the app interface instead of raw interface
    client = AptosCommandSender(backend)
    # Send the special instruction to BOLOS
    response = client.get_app_and_version()
    # Use an helper to parse the response, assert the values
    app_name, version = unpack_get_app_and_version_response(response.data)

    assert app_name == "Aptos"
    verify_version(version)
