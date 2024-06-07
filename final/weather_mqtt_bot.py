appid = '9d763e7cd096892b5a50b5df25faf8ed'  # полученный при регистрации на OpenWeatherMap.org. Что-то вроде такого набора букв и цифр: '6d8e495ca73d5bbc1d6bf8ebd52c4123'

import requests
import serial, serial.tools.list_ports
import time, random
import paho.mqtt.client as mqtt_client
import paho.mqtt.subscribe as subscribe



# def find_COM_port():
#     try:
#         # Find and open the COM port
#         ports = serial.tools.list_ports.comports()
#         port = next((p.device for p in ports), None)
#         if port is None:
#             raise ValueError("No COM port found.")
#         arduino = serial.Serial(port, baudrate=9600)
#     except ValueError as ve:
#         print("Error:", str(ve))
#     except serial.SerialException as se:
#         print("Serial port error:", str(se))
#     except Exception as e:
#         print("An error occurred:", str(e))
#     return arduino


# -----------------------------------------------------------
def get_city_id(s_city_name):
    # Проверка наличия в базе информации о нужном населенном пункте
    try:
        res = requests.get('http://api.openweathermap.org/data/2.5/find',
                           params={'q': s_city_name, 'type': 'like', 'units': 'metric', 'lang': 'ru', 'APPID': appid})
        data = res.json()
        cities = ['{} ({})'.format(d['name'], d['sys']['country'])
                  for d in data['list']]
        # print('city:', cities)
        city_id = data['list'][0]['id']
        # print('city_id=', city_id)
    except Exception as e:
        print('Exception (find):', e)
        pass
    assert isinstance(city_id, int)
    return city_id


def request_current_weather(city_id):
    # Запрос текущей погоды
    weather_descriptions_array = ['Clear', 'Clouds', 'Rain']  # номер погоды от 0 до 2, три варианта
    weather_descriptions_array = ['Clear', 'Clouds', 'Rain', 'Snow', 'Atmosphere', 'Drizzle',
                                  'Thunderstorm']  # все варианты, 3-7 отмечаются как дождь (2)
    try:
        res = requests.get('http://api.openweathermap.org/data/2.5/weather',
                           params={'id': city_id, 'units': 'metric', 'lang': 'ru', 'APPID': appid})
        data = res.json()
        # weather = data['weather'][0]['description'] # точное описание погоды
        weather = data['weather'][0]['main']  # общее описание погоды
        weather_num = weather_descriptions_array.index(weather)
        if (weather_num > 2):
            weather_num = 2
        temperature = data['main']['temp']
        # print('conditions:', weather)
        # print('номер в массиве:', weather_num)
        # print('temp:', temperature)
        # print('data:', data)
        return [temperature, weather_num]
    except Exception as e:
        print('Exception (weather):', e)
        pass


def get_weather(city):
    # city = 'Иркутск'
    # city = 'Москва'
    city_id = get_city_id(city)
    temperature, weather_num = request_current_weather(city_id)

    separator = ','
    # temperature = int(temperature)
    weather_string = f'{temperature}{separator}{weather_num}'
    return [temperature, weather_num]
    # weather_data = weather_num
    # return weather_data


# -----------------------------------------------------------
def send_MQTT(data):
    value_from_arduino = serial_write(str(data))
    print(f'MQTT data = {data}')
    # print(f'MQTT data from arduino = {value_from_arduino}')


def on_message(client, userdata, message):
    global LM_state
    LM_state = str(message.payload.decode("utf-8"))
    print(LM_state)
    # send_MQTT(LM_state)


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print("Failed to connect, return code %d\n", rc)


def mqtt_connection(topic):
    client = mqtt_client.Client(f'client_{random.randint(10000, 99999)}')
    client.on_message = on_message
    client.on_connect = on_connect

    client.connect(broker)
    client.loop_start()
    client.subscribe(topic)
    # client.publish(topic, value)
    time.sleep(50)
    client.disconnect()
    client.loop_stop()


# -----------------------------------------------------------
def send_weather():
    weather_str = get_weather()
    value_from_arduino = serial_write(str(weather_str))
    print(f'weather = {weather_str}')
    # print(f'weather from arduino = {value_from_arduino}')


def send_mqtt_and_weather(LM_state, city):
    LM_states = ['Погода', 'Белый', 'Радуга', 'Синий', 'Конфетти', 'Красный', 'Синелон', 'Голубой', 'Джагл']
    separator = ','
    print(f'Текущий режим: ({LM_state}){LM_states[LM_state]}')

    if LM_state == 0:
        temperature, weather_num = get_weather(city)
        serial_string = f'{LM_state}{separator}{temperature}{separator}{weather_num}'
    else:
        serial_string = f'{LM_state}{separator}0{separator}0'
    value_from_arduino = serial_write(serial_string)
    print(f'По serial отправлена строка {serial_string}')
    # print(f'string from arduino = {value_from_arduino}')


# -----------------------------------------------------------
def serial_write(data):
    arduino.write(bytes(data, 'utf-8'))


def serial_read():
    data = arduino.readline()
    return data


# -----------------------------------------------------------

def find_COM_port():
    # подключение к порту Arduino
    for COM_port in serial.tools.list_ports.comports():
        # print(COM_port.device)
        COM_port = COM_port.device
        try:
            arduino = serial.Serial(port=COM_port, baudrate=9600)
        except:
            continue
    return arduino

def send_serial(city):
    LM_state = 0
    # while True:
    #    city = get_city()
    #    print(city)
    # LM_state = subscribe.simple(LM_topic, hostname=broker)
    send_mqtt_and_weather(int(float(LM_state)), city)
    # send_mqtt_and_weather(int(float(LM_state.payload)), city)


# broker = "broker.emqx.io"
# LM_topic = "leap_motion_states"

# LM_state = 0
# previous_LM_state = 0
# LM_states = ['Погода', 'Белый', 'Радуга', 'Синий', 'Конфетти', 'Красный', 'Синелон', 'Голубой', 'Джагл']

# client = mqtt_client.Client(f'client_{random.randint(10000, 99999)}')
# client.on_message = on_message
# client.on_connect = on_connect
# client.connect(broker)
# client.loop_start()


# # city = input('Введите город: ')
# city = 'Иркутск'
# previous_city='Иркутск'

arduino = find_COM_port()

# while True:
#     LM_state = subscribe.simple(LM_topic, hostname=broker)
#     if LM_state==0:
#         city = 'Иркутск'
#         if previous_city!=city:
#             pass
#     if previous_LM_state!=int(float(LM_state.payload)):
#         previous_LM_state = int(float(LM_state.payload))
#         send_mqtt_and_weather(int(float(LM_state.payload)), city)

