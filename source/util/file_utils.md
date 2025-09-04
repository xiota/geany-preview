# FileUtils – Quick Usage

## Functions

### `readFileToString(path)`
Reads the entire file into a `std::string`.  
Throws `std::runtime_error` if it can’t open the file.

```cpp
std::string css = FileUtils::readFileToString("preview.css");
```

### `watchFile(path, onChange, delayMs = 150)`
Watches a file for changes using `GFileMonitor`.  
- Waits `delayMs` before calling `onChange` (avoids mid‑save reloads).  
- Uses an internal `std::atomic<bool>` to prevent overlapping runs.  
- Returns a `FileWatchHandle` for cleanup.

```cpp
auto handle = FileUtils::watchFile("preview.css", []() {
    reloadCss();
});
```

### `stopWatching(handle)`
Stops monitoring and cleans up.

```cpp
FileUtils::stopWatching(handle);
```

## Typical Pattern
```cpp
baselineWatch = FileUtils::watchFile(configDir() / "preview.css", [this]() { reloadCss(); });
formatWatch   = FileUtils::watchFile(configDir() / (format + ".css"), [this]() { reloadCss(); });

// later...
FileUtils::stopWatching(baselineWatch);
FileUtils::stopWatching(formatWatch);
```
