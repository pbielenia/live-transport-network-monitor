# Useful commands

## Project setup

### Create a Python vitrual environment
```bash
python3 -m venv venv
```

### Activate a Python virtual environment
```bash
source venv/bin/activate
```

### Install Python dependencies
```bash
python -m pip install -r requirements.txt
```

### Install conan packages
Should be run whenever the `requires` field in `conanfile.py` is updated.
```bash
conan install -if build --profile conanprofile.toml .
```

### Generate the project
```bash
cmake -Bbuild -GNinja
ninja -Cbuild
```

### Run all tests
```bash
ninja -Cbuild test
```

### Run tests with log messages enabled
```
./build/network-monitor-tests --log_level=message
```