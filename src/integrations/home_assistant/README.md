# Home Assistant adapter

Keep MQTT credentials in `include/config/Secrets.h`, copied from `Secrets.h.example`.

Use `mqtt_statestream` in Home Assistant for state updates. Put the physical location in Home Assistant:

```yaml
homeassistant:
  latitude: 28.56533
  longitude: 77.37966
  elevation: 200
  time_zone: Asia/Kolkata
```

When importing `papers3-dashboard`, retain its MQTT models/pages but remove its standalone `setup()`, `loop()`, SD initialization, and Wi-Fi initialization. Those belong to the shared services.
