import os

def compare_files(file1, file2):
    """Compara dois arquivos e retorna True se forem iguais, False se forem diferentes."""
    try:
        with open(file1, 'r') as f1, open(file2, 'r') as f2:
            content1 = f1.read()
            content2 = f2.read()
            return content1 == content2
    except Exception as e:
        return False

def compare_out_and_result_files(dir1, dir2):
    """Compara arquivos .out de dir1 com arquivos .result de dir2, com base no nome do arquivo."""
    # Lista todos os arquivos nas duas pastas
    files1 = {f for f in os.listdir(dir1) if f.endswith('.out')}
    files2 = {f.replace('.out', '.result') for f in os.listdir(dir2) if f.endswith('.result')}

    # Encontra os arquivos .out de dir1 com os arquivos .result de dir2 com o mesmo nome base
    for file_name in files1:
        file2_name = file_name.replace('.out', '.result')
        file1_path = os.path.join(dir1, file_name)
        file2_path = os.path.join(dir2, file2_name)

        if file2_name in files2:
            if compare_files(file1_path, file2_path):
                pass  # Arquivos são iguais, não precisa de ação
            else:
                # Arquivos são diferentes, exibe a diferença
                print(f"Os arquivos {file_name} e {file2_name} são DIFERENTES.")
                # Exibe o conteúdo das diferenças
                with open(file1_path, 'r') as f1, open(file2_path, 'r') as f2:
                    content1 = f1.readlines()
                    content2 = f2.readlines()
                    
                    # Exibe as diferenças linha por linha
                    for line1, line2 in zip(content1, content2):
                        if line1 != line2:
                            print(f" - {line1.strip()}")  # Linha do arquivo 1
                            print(f" + {line2.strip()}")  # Linha do arquivo 2
        else:
            pass  # Arquivo correspondente não encontrado

if __name__ == "__main__":
    dir1 = "/home/semed/IST/SO/testsSOp1/jobs"  # Caminho da primeira pasta
    dir2 = "/home/semed/IST/SO/testsSOp1/results"  # Caminho da segunda pasta

    compare_out_and_result_files(dir1, dir2)

