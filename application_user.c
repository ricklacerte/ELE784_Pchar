#include <stdio.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <string.h>
#include <stdlib.h>
#include "ioctl_cmd.h"
#include <sys/ioctl.h>

#define BUF_SIZE 256

char * app_buf = NULL;
char commande='0';
int nb_data=0;
int hd_pilote;
int data_trnsf;
int i=0;
int data;
int res;

void affiche_menu(){
	
	app_buf= (char *)malloc (sizeof (char) * BUF_SIZE);

	printf("\n\n\n\nsalut chummé! Fais un choix judicieux parmi ceux-ci :\n");
		printf("	NO      : ACTIONS \n");
		printf("	--------|---------------------------------- \n");
		printf("	1	: LECTURE en mode Bloquant\n");
		printf("	2	: LECTURE en mode NON-Bloquant\n");
		printf("	3	: ECRITURE en mode Bloquant\n");
		printf("	4	: ECRITURE en mode NON-Bloquant\n\n");
		printf("	5	: IOCTL GetNumData\n");
		printf("	6	: IOCTL GetNumReader\n");
		printf("	7	: IOCTL GetBufSize\n");
		printf("	8	: IOCTL SetBufSize\n\n");
		printf("	m	: afficher le menu\n");
		printf("	q	: quitter\n");
}

int main (int argc, char *argv[]){

affiche_menu();
printf("\n\n\nEntrez votre commande:");

//COMMANDE
while(commande!='q'){

	strcpy(app_buf,"");	
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
			app_buf[data_trnsf]='\0';
			printf("\nnombre de data recue: %d \n",data_trnsf);
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
		scanf(" %d",&nb_data);

		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY | O_NONBLOCK),S_IRUSR);
		if(hd_pilote>0){
			data_trnsf=read(hd_pilote,app_buf,nb_data);
			close(hd_pilote);
			app_buf[data_trnsf]='\0';
			printf("\nnombre de data lue: %d \n",data_trnsf);
			nb_data=strlen(app_buf);
			printf("nb data de la string : %d \n",nb_data);	
			app_buf[data_trnsf]='\0';			
			printf("data : %s \n",app_buf);
	
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

		scanf("%s",app_buf);

		hd_pilote=open("/dev/etsele_cdev",(O_WRONLY | O_NONBLOCK),S_IWUSR);
		if(hd_pilote>0){
			nb_data=strlen(app_buf);
			printf("nombre de data a écrire: %d \n",nb_data);
			data_trnsf=write(hd_pilote,app_buf,nb_data);
			close(hd_pilote);
			printf("nombre de data écrit: %d \n",data_trnsf);		
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;}

	case '5':{
		printf("\n\n\nIOCTL : GetNumData\n");
		printf("-----------------------------------------\n");
		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY | O_NONBLOCK),S_IRUSR);
		if(hd_pilote>0){		
			res=ioctl(hd_pilote,GET_NUM_DATA,&data);
			close(hd_pilote);
			
			printf("data: %d \n",data);
			printf("res: %d \n",res);
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;
		}

	case '6':{
		printf("\n\n\nIOCTL : GetNumReader\n");
		printf("-----------------------------------------\n");
		
		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY | O_NONBLOCK),S_IRUSR);
		if(hd_pilote>0){		
			res=ioctl(hd_pilote,GET_NUM_READER,&data);
			close(hd_pilote);
			printf("data: %d \n",data);
			printf("res: %d \n",res);
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;
		}

	case '7':{
		printf("\n\n\nIOCTL : GetBufSize\n");
		printf("-----------------------------------------\n");
		
		hd_pilote=open("/dev/etsele_cdev",(O_RDONLY | O_NONBLOCK),S_IRUSR);
		if(hd_pilote>0){		
			res=ioctl(hd_pilote,GET_BUF_SIZE,&data);
			close(hd_pilote);
			printf("data: %d \n",data);
			printf("res: %d \n",res);
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;
		}

	case '8':{
		printf("\n\n\nIOCTL : SetBufSize\n");
		printf("-----------------------------------------\n");
		printf("Entrez la nouvelle valeur du Buffer: ");
		scanf("%d",&data);
		hd_pilote=open("/dev/etsele_cdev",(O_WRONLY | O_NONBLOCK),S_IWUSR);
		if(hd_pilote>0){		
			res=ioctl(hd_pilote,SET_BUF_SIZE,&data);
			close(hd_pilote);
			printf("data: %d \n",data);
			printf("res: %d \n",res);
		}
		else{
			printf("incapable d'ouvrir le pilote! \n");
		}
		printf("-----------------------------------------\n");
		break;
		}
	case 'm':{
		affiche_menu();
		break;}

	case 'q':{
		printf("\nBonne journée mon pote !\n\n\n");
		break;}

	default:{
		printf("\nEntrez votre commande:");
	 	break;}
	}
	
}
free(app_buf);
return 0;

}
