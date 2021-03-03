# SmartLaundry
A Sensor to detect when the washing machine has finished and is ready to extend the laundry.
Using
- Wemos D1 Mini
- ACS712 - Hall sensor
- HDC1080 - i2c Temperature / Humidity sensor
- SDD1306 - i2c 0.96 Oled Display (128x64)
- Push button
- Wires
- Wall socket

# Breadboard
![Smart Wash Machine](https://github.com/kadu/SmartWashMachine/blob/main/docs/lavanderia_bb.png?raw=true)

# TODO
- [X] Programação básica do sensor
- [X] Detecção de "maquina funcionando"
- [X] Mostrar avisos no display (Maquina Funcionando | Acabou | Pode extender a roupa)
- [X] Botão para avisar que já extendeu a roupa
- [X] Sistema de mensageria (MQTT)
- [ ] Testar na maquina de verdade
- [ ] Programar a automação (regra de aviso de fim de "lavagem")
- [ ] Fazer a Case 3D
- [ ] Fazer a placa
- [ ] Fazer a Documentaçao