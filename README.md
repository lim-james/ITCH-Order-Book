# itch

A low-latency NASDAQ ITCH 5.0 market data pipeline. Replays binary feed files through a lock-free SPSC queue into per-symbol order books, reconstructing full price-level depth in real time.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/bench <path-to-itch-file>
```

For debug build with sanitizers:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```
