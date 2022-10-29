from importlib.metadata import requires
from conans import ConanFile


class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.80.0'),
        ('openssl/3.0.5'),
        ('libcurl/7.85.0'),
        ('zlib/1.2.13'),
        ('nlohmann_json/3.11.2'),
    ]

    default_options = (
        'boost:shared=False',
    )
