import os
import argparse
from pathlib import Path

texturexml = """<texture>
	<mipmap enable="false" />
	<quality high="0" />
	<srgb enable="false" />
</texture>
"""

# Créez un objet ArgumentParser
parser = argparse.ArgumentParser(description="Description de votre programme")

# Définissez des arguments de ligne de commande
parser.add_argument('--r', action='store_true', help="recursif")

# Analysez les arguments de la ligne de commande
args = parser.parse_args()

# Accédez aux valeurs des arguments
print("recursivité      : ", args.r)

# Spécifiez le chemin du répertoire que vous souhaitez parcourir
repertoire = os.getcwd()

def testerFichierXML(chemin_fichier):

    if not os.path.exists(chemin_fichier):
        return True
    
    with open(chemin_fichier, 'r') as fichier:
        premiere_ligne = fichier.readline().rstrip('\r\n')
        if premiere_ligne == "<texture>":
            return True
    
    return False
    
if args.r == True:
    for root, _, fichiers in os.walk(repertoire):
        for fichier in fichiers:
            chemin_fichier = os.path.join(root, fichier)
            extension = chemin_fichier.split('.')[-1]
            if extension == "png":
                nom_fichier = os.path.join(root, f'{fichier[:-4]}.xml')
                if testerFichierXML(nom_fichier):
                    print(f'Add xml file to texture : {root}/{fichier}')
                    with open(nom_fichier, "w") as f:
                        f.write(texturexml)                                
else:
    root = Path(repertoire)
    # Parcourez le répertoire pour obtenir les fichiers
    for fichier in root.iterdir():
        if fichier.is_file():
            extension = fichier.name.split('.')[-1]
            if extension == "png":
                nom_fichier = os.path.join(root, f'{fichier.name[:-4]}.xml')
                if testerFichierXML(nom_fichier):
                    print(f'Add xml file to texture : {root}/{fichier.name}')            
                    with open(nom_fichier, "w") as f:
                        f.write(texturexml)        


