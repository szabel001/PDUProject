import pandas as pd

INPUT_CSV = "modbus_registers.csv"
OUTPUT_HEADER = "IECRegisterMap.h"

REGISTER_SECTIONS = {
    "Coil": "----------------Coils------------------",
    "Discrete Input": "-------------Discrete inputs---------------",
    "Input Register": "-------------Input registers---------------",
    "Holding Register": "-------------Holding registers---------------",
}

def parse_address(addr):
    if isinstance(addr, str):
        addr = addr.strip()
        if addr.lower().startswith("0x"):
            return int(addr, 16)
        else:
            return int(addr)
    return int(addr)

def format_define_addr(name, addr, description):
    desc = description.strip() if pd.notna(description) else ""
    return f"#define {name.upper()}_ADDR 0x{addr:04X} // {desc}"

def generate_header(df):
    lines = []
    lines.append("#ifndef IECRegisterMap_h")
    lines.append("#define IECRegisterMap_h\n")

    for reg_type, title in REGISTER_SECTIONS.items():
        section_df = df[df["Register"].str.lower().str.contains(reg_type.lower(), na=False)]
        if section_df.empty:
            print('No entries for register type:', reg_type)
            continue

        section_df["Address_num"] = section_df["Address"].apply(parse_address)
        section_df = section_df.sort_values(by="Address_num")

        lines.append(f"/*\n{title}\n*/")
        for _, row in section_df.iterrows():
            name = str(row["Name"]).strip()
            addr = row["Address_num"]
            description = row.get("Description", "")
            print(name, addr, description)
            lines.append(format_define_addr(name, addr, description))
        lines.append("")

    lines.append("#endif")
    return "\n".join(lines)

def main():
    df = pd.read_csv(INPUT_CSV, sep=';')
    df.columns = [c.strip() for c in df.columns]
    required_cols = ["Name", "Address", "Register", "Description"]
    for col in required_cols:
        if col not in df.columns:
            raise ValueError(f"Missing column in CSV: {col}")

    header_content = generate_header(df)

    with open(OUTPUT_HEADER, "w", encoding="utf-8") as f:
        f.write(header_content)

    print(f"Header file generated successfully: {OUTPUT_HEADER}")

if __name__ == "__main__":
    main()
