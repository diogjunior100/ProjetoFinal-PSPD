import subprocess
import random
from concurrent.futures import ThreadPoolExecutor

# Function to run the command
def run_command(valor1, valor2):
    comando = ['python3', 'teste_client.py', str(valor1), str(valor2)]
    print(f"Executing: python3 teste_client.py {valor1} {valor2}")
    subprocess.run(comando)

# Number of executions
num_execucoes = 100

# Create a ThreadPoolExecutor to run commands in parallel
with ThreadPoolExecutor() as executor:
    for _ in range(num_execucoes):
        # Ensure the first number is smaller than the second
        valor1 = random.randint(4, 9)  # The first number is between 4 and 9
        valor2 = random.randint(valor1 + 1, 10)  # The second number is between valor1+1 and 10
        
        # Submit the task to be run in parallel
        executor.submit(run_command, valor1, valor2)
