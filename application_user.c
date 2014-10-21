#include <stdio.h>
#include <fcntl.h>
#include <linux/stat.h>
//#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 256

char * app_buf;
char commande='0';
int nb_data=0;
int hd_pilote;
int data_trnsf;

void affiche_menu(){
	app_buf=(char *)malloc (sizeof (char) * BUF_SIZE);
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
printf("\n\n\nEntrez votre commande:");

//COMMANDE
while(commande!='q'){
	scanf("%c",&commande);

	switch (commande) {
	case '1':{
		printf("\n\n\nLECTURE BLOQUANTE\n");
		printf("-----------------------------------------\n");
		printf("Entrez le nombre de données à lire: ");
		scanf("%d",&nb_data);

		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY),S_IRUSR);
		if(hd_pilote>0){
			data_trnsf=read(hd_pilote,app_buf,nb_data);
			printf("nombre de data recue: %d \n",data_trnsf);
			printf("data : %s \n",app_buf);
			close(hd_pilote);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;}

	case '2':{
		printf("\n\n\nLECTURE NON-BLOQUANTE\n");
		printf("-----------------------------------------\n");
		printf("Entrez le nombre de données à lire: ");
		scanf("%d",&nb_data);

		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY | O_NONBLOCK),S_IRUSR);
		if(hd_pilote>0){
			data_trnsf=read(hd_pilote,app_buf,nb_data);
			printf("nombre de data recue: %d \n",data_trnsf);
			printf("data : %s \n",app_buf);
			close(hd_pilote);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
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
		printf("Bonne journée mon pote !\n");
		break;}

	default:{
		printf("Entrez votre commande:");
	 	break;}
	}
}
free(app_buf);
return 0;

}
