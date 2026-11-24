import matplotlib.pyplot as plt

# Datos experimentales
procesos = [1, 2, 4, 8, 12]
tiempo_secuencial = 0.2202

tiempos_mpi = [
    0.226168,
    0.217465,
    0.218471,
    0.298309,
    0.283243
]

# Calcular speedup
speedup = [tiempo_secuencial / t for t in tiempos_mpi]

# Speedup ideal
ideal = procesos

# Crear gráfica
plt.figure(figsize=(9, 5.5))

plt.plot(
    procesos, speedup,
    marker='o',
    linewidth=2,
    label='Speedup obtenido'
)

plt.plot(
    procesos, ideal,
    linestyle='--',
    marker='s',
    label='Speedup ideal'
)

plt.title('Ejercicio 5 - Speedup de la difusión de calor')
plt.xlabel('Número de procesos')
plt.ylabel('Speedup')
plt.xticks(procesos)
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()

# Guardar imagen
plt.savefig('grafica_speedup.png', dpi=300)
plt.show()

print('Speedup obtenido:')
for p, s in zip(procesos, speedup):
    print(f'{p} procesos: {s:.3f}')
