#include <stdio.h>

#define BUF_SIZE 16
#define CMD_SIZE 50

char app_buf[BUF_SIZE]={0};
char commande='0';
int nb_data=0;

void affiche_menu(){
	printf("\n\n\n\nsalut chummé! Fais un choix judicieux parmi ceux-ci :\n");
		printf("	NO      : ACTIONS \n");
		printf("	--------|---------------------------------- \n");
		printf("	1	: LECTURE en mode Bloquant\n");
		printf("	2	: LECTURE en mode NON-Bloquant\n");
		printf("	3	: ECRITURE en mode Bloquant\n");
		printf("	4	: ECRITURE en mode NON-Bloquant\n");
		printf("	m	: afficher le menu\n");
		printf("	q	: quitter\n");
}

int main (int argc, char *argv[]){

affiche_menu();
printf("Entrez votre commande:");

//COMMANDE
while(commande!='q'){
	scanf("%c",&commande);

	switch (commande) {
	case '1':{
		printf("READ_B\n");
		printf("Entrez le nombre de données à lire: ");
		scanf("%d",&nb_data);
		open("/dev/etsele_cdev",gfdshghuytreuyhtgnhtewytrv g kjuytojn
		
		break;}

	case '2':{
		printf("READ_NB\n");
		printf("Entrez le nombre de données à lire: ");
		scanf("%d",&nb_data);
		break;}

	case '3':{
		printf("WRITE_B\n");
		break;}

	case '4':{
		printf("WRITE_NB\n");
		break;}

	case 'm':{
		affiche_menu();
		break;}

	case 'q':{
		printf("Bonne journée, vous venez de quitter!\n");
		break;}

	default:{
		printf("Entrez votre commande:");
	 	break;}
	}
}
return 0;

}
