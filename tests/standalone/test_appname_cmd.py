from application_client.boilerplate_command_sender import BoilerplateCommandSender
from application_client.boilerplate_response_unpacker import (
    unpack_get_app_name_response,
)


# In this test we check that the GET_APP_NAME replies the application name
def test_app_name(backend):
    # Use the app interface instead of raw interface
    client = BoilerplateCommandSender(backend)
    # Send the GET_APP_NAME instruction to the app
    app_name_apdu = client.get_app_name()
    assert unpack_get_app_name_response(app_name_apdu.data) == "Aptos"
