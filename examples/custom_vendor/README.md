# Custom Vendor SDK

Demonstrates:
- Using `VendorSDK` and `VendorSDKFactory` to configure a custom vendor
- Registering vendor metadata (USB IDs, protocols, etc.)
- Registering a custom protocol with transport bindings
- Registering a custom USB transport
- Registering workflows, jobs, package formats, and discovery methods
- Registering capabilities
- Finalizing the SDK configuration and inspecting the report

## Build

```bash
cd build
cmake --build . --target example_custom_vendor
```

## Run

```bash
./example_custom_vendor
```
