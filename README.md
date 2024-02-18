# brenner-outside-temp
# Oil Heater outside temperature IoT sensor

Oil heater control unit uses outside temperature
to turn on standby mode when the outside temperature
reaches defined value (18degC).

An Arduino board listens for a temperature change
and sets digital potenttiometer that is connected
to an oil heater's control unit.

Board: Seeed Studion XIAO ESP32C3
Digital pot: PmodDPOT from DIGILENT
  - 8-bit resolution
  - 60Ohm - 10kOhm range
  - controlled through SPI

## define an action in HA config (automations.yaml)
This action runs when outside temperature changes and
sends it to the device:
```
- id: NEW_ID
  alias: Outside temperature changed
  description: ''
  trigger:
  - platform: state
    entity_id:
    - weather.home
    attribute: temperature
  condition: []
  action:
  - service: number.set_value
    target:
      entity_id: number.oil_heater_outside_temperatur
    data:
      value: '{{ state_attr(''weather.home'', ''temperature'') }}'
```