
#include "initStack.h"

#define CAN_EFF_FLAG 0x80000000U

enum sentence sentence_id  (int Id, int Pgn)
{
	if( (unsigned int)Id < CAN_EFF_FLAG)
		return ID_INVALID;

	else
		Pgn = (Id & 0x00ffff00) >> 8;

	switch  (Pgn)
	{
		case 0xfef1 :
		return PGN_SPEED_VEHICLE;
		break;
	}

}

bool extractVitesse (Tst_j1939Buf *extracting_param) {

	switch (sentence_id(extracting_param->id, extracting_param->Pgn))
	{

		case PGN_SPEED_VEHICLE : {
			extracting_param->data[0] = extracting_param->data[2];
			extracting_param->vitesse = ((extracting_param->data[0] << 8) + (extracting_param->data[1]))/ 256;
			printf ("Vitesse = %d km/h \n", extracting_param->vitesse);
		} break;

		case ID_INVALID :
		printf(INDENT_SPACES "Id is not valid\n");
		break;

		default:
		printf(INDENT_SPACES "Id unknown\n");
		break;
	}


	return true;
}

