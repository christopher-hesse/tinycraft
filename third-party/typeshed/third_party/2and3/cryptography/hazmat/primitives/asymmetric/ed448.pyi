from abc import ABCMeta, abstractmethod

from cryptography.hazmat.primitives.serialization import (
    Encoding,
    KeySerializationEncryption,
    PrivateFormat,
    PublicFormat,
)

class Ed448PrivateKey(metaclass=ABCMeta):
    @classmethod
    def generate(cls) -> Ed448PrivateKey: ...
    @classmethod
    def from_private_bytes(cls, data: bytes) -> Ed448PrivateKey: ...
    @abstractmethod
    def private_bytes(
        self,
        encoding: Encoding,
        format: PrivateFormat,
        encryption_algorithm: KeySerializationEncryption,
    ) -> bytes: ...
    @abstractmethod
    def public_key(self) -> Ed448PublicKey: ...
    @abstractmethod
    def sign(self, data: bytes) -> bytes: ...

class Ed448PublicKey(metaclass=ABCMeta):
    @classmethod
    def from_public_bytes(cls, data: bytes) -> Ed448PublicKey: ...
    @abstractmethod
    def public_bytes(self, encoding: Encoding, format: PublicFormat) -> bytes: ...
    @abstractmethod
    def verify(self, signature: bytes, data: bytes) -> None: ...
