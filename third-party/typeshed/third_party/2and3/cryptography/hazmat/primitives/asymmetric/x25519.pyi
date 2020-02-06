from abc import ABCMeta, abstractmethod

from cryptography.hazmat.primitives.serialization import (
    Encoding,
    KeySerializationEncryption,
    PrivateFormat,
    PublicFormat,
)

class X25519PrivateKey(metaclass=ABCMeta):
    @classmethod
    def from_private_bytes(cls, data: bytes) -> X25519PrivateKey: ...
    @classmethod
    def generate(cls) -> X25519PrivateKey: ...
    @abstractmethod
    def exchange(self, peer_public_key: X25519PublicKey) -> bytes: ...
    @abstractmethod
    def private_bytes(
        self,
        encoding: Encoding,
        format: PrivateFormat,
        encryption_algorithm: KeySerializationEncryption,
    ) -> bytes: ...
    @abstractmethod
    def public_key(self) -> X25519PublicKey: ...

class X25519PublicKey(metaclass=ABCMeta):
    @classmethod
    def from_public_bytes(cls, data: bytes) -> X25519PublicKey: ...
    @abstractmethod
    def public_bytes(self, encoding: Encoding, format: PublicFormat) -> bytes: ...
