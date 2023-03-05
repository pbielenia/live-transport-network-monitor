from conans import ConanFile


class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"

    generators = 'cmake_find_package'

    requires = [
        ('boost/1.80.0'),
        ('libcurl/7.85.0'),
        ('nlohmann_json/3.11.2'),
        ('openssl/1.1.1h'),
        ('zlib/1.2.13'),
    ]

    default_options = (
        'boost:shared=False',
    )
