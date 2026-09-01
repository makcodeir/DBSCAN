# DBSCAN

An open-source implementation of the DBSCAN clustering algorithm in C++.

## What is DBSCAN?

DBSCAN (Density-Based Spatial Clustering of Applications with Noise) is a density-based
clustering algorithm. Instead of grouping points around centroids like k-means, it
clusters points that are packed closely together and marks isolated points as noise.

Two parameters control the algorithm:

| Parameter | Meaning |
|---|---|
| `eps` | The neighborhood radius: how close two points must be to be considered neighbors |
| `minPts` | The minimum number of points required to form a dense region |

Each point is classified as one of:

- **Core point** — has at least `minPts` neighbors (including itself) within `eps`.
- **Border point** — within `eps` of a core point, but not dense itself.
- **Noise point** — neither a core nor a border point.

Clusters are grown from core points: all points reachable from a core point (through
chains of neighboring core points) end up in the same cluster. This lets DBSCAN find
clusters of arbitrary shape — not just spherical ones — and it does not require you to
know the number of clusters in advance, unlike k-means.

## Status

Work in progress. Currently implemented:

- `point` — an N-dimensional point with Euclidean distance (`point.cpp`).

Planned: the core DBSCAN loop (neighbor search, cluster expansion, noise labeling).

## Build and run the demo

```
cmake -S . -B build
cmake --build build
./build/csv_adapter_demo .
```

## Side quest: CSV adapter (`csv_adapter.hpp`)

As a supporting piece, the repo ships a schema-driven CSV adapter that loads CSV files
and maps rows into C++ value classes — handy for feeding datasets into the clustering.

The short version: you describe your data class in a `*.schema` file (field names and
types), scan a folder of schemas, register a factory for your class, and the adapter
produces your objects from CSV rows. See `csv_adapter.hpp` and `main.cpp` for the full
demo covering schemas, delimiters, and error handling.

Requires C++17.
