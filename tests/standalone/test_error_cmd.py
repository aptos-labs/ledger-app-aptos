import pytest
from application_client.boilerplate_command_sender import CLA, P1, P2, Errors, InsType
from ragger.error import ExceptionRAPDU


# Ensure the app returns an error when a bad CLA is used
def test_bad_cla(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA + 1, ins=InsType.GET_VERSION)
    assert e.value.status == Errors.SW_CLA_NOT_SUPPORTED


# Ensure the app returns an error when a bad INS is used
def test_bad_ins(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=0xFF)
    assert e.value.status == Errors.SW_INS_NOT_SUPPORTED


# Ensure the app returns an error when a bad P1 or P2 is used
def test_wrong_p1p2(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=InsType.GET_VERSION, p1=P1.P1_START + 1, p2=P2.P2_LAST)
    assert e.value.status == Errors.SW_WRONG_P1P2
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=InsType.GET_VERSION, p1=P1.P1_START, p2=P2.P2_MORE)
    assert e.value.status == Errors.SW_WRONG_P1P2
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=InsType.GET_APP_NAME, p1=P1.P1_START + 1, p2=P2.P2_LAST)
    assert e.value.status == Errors.SW_WRONG_P1P2
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=InsType.GET_APP_NAME, p1=P1.P1_START, p2=P2.P2_MORE)
    assert e.value.status == Errors.SW_WRONG_P1P2


# Ensure the app returns an error when a bad data length is used
def test_wrong_data_length(backend):
    # APDUs must contain at least CLA, INS, P1, P2, and Lc.
    missing_lc_apdu = bytes((CLA, InsType.GET_VERSION, P1.P1_START, P2.P2_LAST))
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange_raw(missing_lc_apdu)
    assert e.value.status == Errors.SW_WRONG_DATA_LENGTH
    # Lc must not advertise data absent from the APDU.
    missing_data_apdu = bytes((CLA, InsType.GET_VERSION, P1.P1_START, P2.P2_LAST, 5))
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange_raw(missing_data_apdu)
    assert e.value.status == Errors.SW_WRONG_DATA_LENGTH
    # v1 accepts only a one-byte Lc, never an extended-length APDU.
    extended_apdu = bytes((CLA, InsType.GET_VERSION, P1.P1_START, P2.P2_LAST, 0, 0, 1, 0))
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange_raw(extended_apdu)
    assert e.value.status == Errors.SW_WRONG_DATA_LENGTH
