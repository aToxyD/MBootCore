# Backup Partition

Demonstrates:
- Discovering and connecting to a device
- Reading partition contents via `Runtime::readPartition()`
- Saving partition data to a binary file
- Restoring partition data from a backup file via `Runtime::restore()`
- Pre-restore backup creation
- File I/O and verification

## Build

```bash
cd build
cmake --build . --target example_backup_partition
```

## Run

```bash
# Backup mode
./example_backup_partition boot

# Restore mode
./example_backup_partition boot boot_backup.bin
```
