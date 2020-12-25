# Useful commands

## Activate Python virtual environment
```bash
source venv/bin/activate
```

## Install Python dependencies
```bash
pip3 install -r requirements.txt
```

## Install conan packages
```bash
conan install -if build --profile conanprofile.toml .
```

## Generate project
```bash
cmake -Bbuild -GNinja
ninja -Cbuild
```