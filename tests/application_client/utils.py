from nacl.exceptions import BadSignatureError
from nacl.signing import VerifyKey


# Check whether an Ed25519 signature of a given message is valid.
def check_signature_validity(public_key: bytes, signature: bytes, message: bytes) -> bool:
    try:
        VerifyKey(public_key).verify(message, signature)
    except BadSignatureError:
        return False
    return True
