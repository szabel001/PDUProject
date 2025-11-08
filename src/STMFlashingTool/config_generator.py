import csv
import struct
import os

CSV_FILE = "config_data.csv"
OUTPUT_DIR = "config_bins"
os.makedirs(OUTPUT_DIR, exist_ok=True)

MAGIC = 0xDEADBEEF
STRUCT_FORMAT = "<IIIIfIBBB"

print("Generating CONFIG binaries based on CSV...\n")

with open(CSV_FILE, 'r', encoding='utf-8-sig') as csvfile:
    sample = csvfile.read(2048)
    csvfile.seek(0)
    dialect = csv.Sniffer().sniff(sample)
    reader = csv.DictReader(csvfile, dialect=dialect)

    for row in reader:
        row = {k.strip(): v.strip() for k, v in row.items() if k}
        keys_lower = {k.lower(): k for k in row.keys()}

        def get_field(name, default=None):
            for k in row.keys():
                if k.strip().lower() == name.lower():
                    return row[k]
            return default

        ID = int(get_field("ID", get_field("MODBUS_ID", 0)))
        MODBUS_ID = int(get_field("MODBUS_ID", ID))
        VERSION = float(get_field("VERSION", 0))
        AVAILABLE_LEDS = int(get_field("AVAILABLE_LEDS", 0))
        CURRENT_LIMIT = float(get_field("CURRENT_LIMIT", 0))
        RELAY_COUNT = int(get_field("RELAY_COUNT", 0))
        IS_CURRENT_MEASURED = int(get_field("IS_CURRENT_MEASURED", 0))
        IS_VOLTAGE_MEASURED = int(get_field("IS_VOLTAGE_MEASURED", 0))
        reserved = 0

        packed = struct.pack(
            STRUCT_FORMAT,
            MAGIC, ID, int(VERSION), AVAILABLE_LEDS, CURRENT_LIMIT,
            RELAY_COUNT, IS_CURRENT_MEASURED, IS_VOLTAGE_MEASURED, MODBUS_ID
        ) + bytes([reserved])

        filename = f"config_ID_{ID}.bin"
        path = os.path.join(OUTPUT_DIR, filename)
        with open(path, "wb") as f:
            f.write(packed)

        print(f"{filename} created (ID={ID}, VERSION={VERSION}, MODBUS_ID={MODBUS_ID})")

print("All CONFIG files are generated successfully!")
