#include <stdio.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 256

char * app_buf = NULL;
char commande='0';
int nb_data=0;
int hd_pilote;
int data_trnsf;

void affiche_menu(){
	app_buf= malloc (sizeof (char) * BUF_SIZE);
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
	nb_data=0;
	data_trnsf=0;
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
			printf("nombre de data lue: %d \n",data_trnsf);
			printf("data : %s \n",app_buf);
			close(hd_pilote);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;}



	case '3':{
		printf("\n\n\nÉCRITURE BLOQUANTE\n");
		printf("-----------------------------------------\n");
		printf("Entrez les de données à écrire: ");
		scanf("%s",app_buf);

		hd_pilote=open("/dev/etsele_cdev",(O_WRONLY),S_IWUSR);
		if(hd_pilote>0){
			nb_data=strlen(app_buf);
			data_trnsf=write(hd_pilote,app_buf,nb_data);
			printf("nombre de data écrit: %d \n",data_trnsf);
			close(hd_pilote);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;}

	case '4':{
		printf("\n\n\nÉCRITURE NON-BLOQUANTE\n");
		printf("-----------------------------------------\n");
		printf("Entrez les de données à écrire: ");

		scanf("%s",app_buf);;
		nb_data=strlen(app_buf);

		hd_pilote=open("/dev/etsele_cdev",(O_WRONLY | O_NONBLOCK),S_IWUSR);
		if(hd_pilote>0){
			nb_data=strlen(app_buf);
			data_trnsf=write(hd_pilote,app_buf,nb_data);
			printf("nombre de data écrit: %d \n",data_trnsf);
			close(hd_pilote);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
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
