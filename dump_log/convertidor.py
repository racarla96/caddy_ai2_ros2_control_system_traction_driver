import re

def convertir_candump_a_canplayer(archivo_entrada, archivo_salida):
    with open(archivo_entrada, 'r') as f_in, open(archivo_salida, 'w') as f_out:
        for linea in f_in:
            # Eliminar espacios redundantes
            linea = linea.strip()

            # Expresión regular ajustada para tolerar espacios variables
            match = re.match(
                r'\(([\d.]+)\)\s+(\w+)\s+([0-9A-Fa-f]{3})\s+\[\d+\]\s+((?:[0-9A-Fa-f]{2}\s*)+)', 
                linea
            )

            if match:
                timestamp = match.group(1)
                interfaz = match.group(2)
                can_id = match.group(3)
                data = match.group(4).strip().replace(' ', '')
                f_out.write(f'({timestamp}) {interfaz} {can_id}#{data}\n')
            else:
                # Si la línea no coincide con el patrón, escríbela tal cual (opcional)
                pass  # O puedes usar: f_out.write(linea + '\n')

# Uso: coloca tu archivo de entrada y salida
convertir_candump_a_canplayer('candump-2025_05_19_test2_conector_rojo.log', 'candump-2025_05_19_test2.log')

