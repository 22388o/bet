#include "bet.h"
#include "config.h"
#include "common.h"
#include "misc.h"
#include "err.h"
#include "commands.h"

char *dealer_config_file = "./config/dealer_config.json";
char *player_config_file = "./config/player_config.json";
char *notaries_file = "./config/cashier_nodes.json";

char *dealer_config_ini_file = "./config/dealer_config.ini";
char *player_config_ini_file = "./config/player_config.ini";
char *cashier_config_ini_file = "./config/cashier_config.ini";
char *bets_config_ini_file = "./config/bets.ini";

cJSON *bet_read_json_file(char *file_name)
{
	FILE *fp = NULL;
	cJSON *json_data = NULL;
	char *data = NULL, buf[256];
	unsigned long data_size = 1024, buf_size = 256, temp_size = 0;
	unsigned long new_size = data_size;

	data = calloc(data_size, sizeof(char));
	if (!data) {
		goto end;
	}

	fp = fopen(file_name, "r");
	if (fp == NULL) {
		dlg_error("Failed to open file %s\n", file_name);
		goto end;
	} else {
		while (fgets(buf, buf_size, fp) != NULL) {
			temp_size = temp_size + strlen(buf);
			if (temp_size >= new_size) {
				char *temp = calloc(new_size, sizeof(char));
				strncpy(temp, data, strlen(data));
				free(data);
				new_size = new_size * 2;
				data = calloc(new_size, sizeof(char));
				strncpy(data, temp, strlen(temp));
				free(temp);
			}
			strcat(data, buf);
			memset(buf, 0x00, buf_size);
		}
		json_data = cJSON_CreateObject();
		json_data = cJSON_Parse(data);
	}
end:
	if (data)
		free(data);
	return json_data;
}

void bet_parse_dealer_config_file()
{
	cJSON *config_info = NULL;
	char type[10];

	config_info = bet_read_json_file(dealer_config_file);
	if (config_info) {
		max_players = jint(config_info, "max_players");
		table_stake_in_chips = jdouble(config_info, "table_stake_in_chips");
		chips_tx_fee = jdouble(config_info, "chips_tx_fee");
		dcv_commission_percentage = jdouble(config_info, "dcv_commission");
		strcpy(type, jstr(config_info, "type"));
		dlg_info("The maxplayers for the table set by dealer is :: %d, dealers can change this in the %s",
			 max_players, dealer_config_file);
	}
}

void bet_parse_cashier_nodes_file()
{
	cJSON *notaries_info = NULL;

	notaries_info = bet_read_json_file(notaries_file);
	if (notaries_info) {
		no_of_notaries = cJSON_GetArraySize(notaries_info);
		notary_node_ips = (char **)malloc(no_of_notaries * sizeof(char *));
		notary_node_pubkeys = (char **)malloc(no_of_notaries * sizeof(char *));
		notary_status = (int *)malloc(no_of_notaries * sizeof(int));

		for (int32_t i = 0; i < no_of_notaries; i++) {
			cJSON *node_info = cJSON_CreateObject();
			node_info = cJSON_GetArrayItem(notaries_info, i);

			notary_node_ips[i] = (char *)malloc(strlen(jstr(node_info, "ip")) + 1);
			memset(notary_node_ips[i], 0x00, strlen(jstr(node_info, "ip")) + 1);

			notary_node_pubkeys[i] = (char *)malloc(strlen(jstr(node_info, "pubkey")) + 1);
			memset(notary_node_pubkeys[i], 0x00, strlen(jstr(node_info, "pubkey")) + 1);

			strncpy(notary_node_ips[i], jstr(node_info, "ip"), strlen(jstr(node_info, "ip")));
			strncpy(notary_node_pubkeys[i], jstr(node_info, "pubkey"), strlen(jstr(node_info, "pubkey")));
		}
	}
}

void bet_parse_player_config_file()
{
	cJSON *config_info = NULL;

	config_info = bet_read_json_file(player_config_file);
	if (config_info) {
		max_allowed_dcv_commission = jdouble(config_info, "max_allowed_dcv_commission");
	}
}

void bet_parse_dealer_config_ini_file()
{
	dictionary *ini = NULL;

	ini = iniparser_load(dealer_config_ini_file);
	if (ini == NULL) {
		dlg_error("error in parsing %s", dealer_config_ini_file);
	} else {
		if (-1 != iniparser_getint(ini, "table:max_players", -1)) {
			max_players = iniparser_getint(ini, "table:max_players", -1);
		}
		if (0 != iniparser_getdouble(ini, "table:big_blind", 0)) {
			BB_in_chips = iniparser_getdouble(ini, "table:big_blind", 0);
			SB_in_chips = BB_in_chips / 2;
		}
		if (0 != iniparser_getint(ini, "table:min_stake", 0)) {
			table_min_stake = iniparser_getint(ini, "table:min_stake", 0) * BB_in_chips;
		}
		if (0 != iniparser_getint(ini, "table:max_stake", 0)) {
			table_max_stake = iniparser_getint(ini, "table:max_stake", 0) * BB_in_chips;
		}

		if (0 != iniparser_getdouble(ini, "dealer:chips_tx_fee", 0)) {
			chips_tx_fee = iniparser_getdouble(ini, "dealer:chips_tx_fee", 0);
		}
		if (0 != iniparser_getdouble(ini, "dealer:dcv_commission", 0)) {
			dcv_commission_percentage = iniparser_getdouble(ini, "dealer:dcv_commission", 0);
		}
		if (NULL != iniparser_getstring(ini, "dealer:gui_host", NULL)) {
			strcpy(dcv_hosted_gui_url, iniparser_getstring(ini, "dealer:gui_host", NULL));
		}
		if (-1 != iniparser_getboolean(ini, "private table:is_table_private", -1)) {
			is_table_private = iniparser_getboolean(ini, "private table:is_table_private", -1);
		}
		if (NULL != iniparser_getstring(ini, "private table:table_password", NULL)) {
			strcpy(table_password, iniparser_getstring(ini, "private table:table_password", NULL));
		}
		if (-1 != iniparser_getboolean(ini, "dealer:bet_ln_config", -1)) {
			bet_ln_config = iniparser_getboolean(ini, "dealer:bet_ln_config", -1);
		}
	}
}

void bet_parse_player_config_ini_file()
{
	dictionary *ini = NULL;

	ini = iniparser_load(player_config_ini_file);
	if (ini == NULL) {
		dlg_error("error in parsing %s", player_config_ini_file);
	} else {
		if (0 != iniparser_getdouble(ini, "player:max_allowed_dcv_commission", 0)) {
			max_allowed_dcv_commission = iniparser_getdouble(ini, "player:max_allowed_dcv_commission", 0);
		}
		if (0 != iniparser_getint(ini, "player:table_stake_size", 0)) {
			table_stack_in_bb = iniparser_getint(ini, "player:table_stake_size", 0);
		}
		if (0 != iniparser_getstring(ini, "player:name", NULL)) {
			strcpy(player_name, iniparser_getstring(ini, "player:name", NULL));
		}
		if (-1 != iniparser_getboolean(ini, "private table:is_table_private", -1)) {
			is_table_private = iniparser_getboolean(ini, "private table:is_table_private", -1);
		}
		if (NULL != iniparser_getstring(ini, "private table:table_password", NULL)) {
			strcpy(table_password, iniparser_getstring(ini, "private table:table_password", NULL));
		}
		if (-1 != iniparser_getboolean(ini, "player:bet_ln_config", -1)) {
			bet_ln_config = iniparser_getboolean(ini, "player:bet_ln_config", -1);
		}
	}
}

void bet_parse_cashier_config_ini_file()
{
	cJSON *cashiers_info = NULL;
	dictionary *ini = NULL;

	ini = iniparser_load(cashier_config_ini_file);
	if (ini == NULL) {
		dlg_error("error in parsing %s", cashier_config_ini_file);
	} else {
		char str[20];
		int i = 1;
		sprintf(str, "cashier:node-%d", i);
		cashiers_info = cJSON_CreateArray();
		while (NULL != iniparser_getstring(ini, str, NULL)) {
			cJSON_AddItemToArray(cashiers_info, cJSON_Parse(iniparser_getstring(ini, str, NULL)));
			memset(str, 0x00, sizeof(str));
			sprintf(str, "cashier:node-%d", ++i);
		}
		no_of_notaries = cJSON_GetArraySize(cashiers_info);
		notary_node_ips = (char **)malloc(no_of_notaries * sizeof(char *));
		notary_node_pubkeys = (char **)malloc(no_of_notaries * sizeof(char *));
		notary_status = (int *)malloc(no_of_notaries * sizeof(int));

		for (int32_t i = 0; i < no_of_notaries; i++) {
			cJSON *node_info = cJSON_CreateObject();
			node_info = cJSON_GetArrayItem(cashiers_info, i);

			notary_node_ips[i] = (char *)malloc(strlen(jstr(node_info, "ip")) + 1);
			memset(notary_node_ips[i], 0x00, strlen(jstr(node_info, "ip")) + 1);

			notary_node_pubkeys[i] = (char *)malloc(strlen(jstr(node_info, "pubkey")) + 1);
			memset(notary_node_pubkeys[i], 0x00, strlen(jstr(node_info, "pubkey")) + 1);

			strncpy(notary_node_ips[i], jstr(node_info, "ip"), strlen(jstr(node_info, "ip")));
			strncpy(notary_node_pubkeys[i], jstr(node_info, "pubkey"), strlen(jstr(node_info, "pubkey")));
		}
	}
}

void bet_display_cashier_hosted_gui()
{
	dictionary *ini = NULL;

	ini = iniparser_load(player_config_ini_file);
	if (ini == NULL) {
		dlg_error("error in parsing %s", player_config_ini_file);
	} else {
		char str[20];
		int i = 1;
		sprintf(str, "gui:cashier-%d", i);
		while (NULL != iniparser_getstring(ini, str, NULL)) {
			if (check_url(iniparser_getstring(ini, str, NULL)))
				dlg_warn("%s", iniparser_getstring(ini, str, NULL));
			memset(str, 0x00, sizeof(str));
			sprintf(str, "gui:cashier-%d", ++i);
		}
	}
}

static int32_t ini_sec_exists(dictionary *ini, char *sec_name)
{
	int32_t n, retval = -1;

	n = iniparser_getnsec(ini);
	dlg_info("number of sections:: %d", n);
	for (int32_t i = 0; i < n; i++) {
		dlg_info("%s::%s", iniparser_getsecname(ini, i), sec_name);
		if (strcmp(iniparser_getsecname(ini, i), sec_name) == 0) {
			retval = OK;
			break;
		}
	}
	return retval;
}
int32_t bet_parse_bets()
{
	int32_t retval = OK, bet_no = 0;
	dictionary *ini = NULL;
	char key_name[40];
	cJSON *bets_info = NULL, *info = NULL;

	ini = iniparser_load(bets_config_ini_file);
	if (ini == NULL) {
		retval = ERR_INI_PARSING;
		dlg_error("error in parsing %s", bets_config_ini_file);
		return retval;
	}
	info = cJSON_CreateObject();
	cJSON_AddStringToObject(info, "method", "bets");
	cJSON_AddNumberToObject(info, "balance", chips_get_balance());
	bets_info = cJSON_CreateArray();
	while (1) {
		cJSON *bet = cJSON_CreateObject();

		cJSON_AddNumberToObject(bet, "bet_id", bet_no);
		memset(key_name, 0x00, sizeof(key_name));
		sprintf(key_name, "bets:%d:desc", bet_no);
		if (NULL != iniparser_getstring(ini, key_name, NULL)) {
			cJSON_AddStringToObject(bet, "desc", iniparser_getstring(ini, key_name, NULL));
		} else {
			break;
		}
		memset(key_name, 0x00, sizeof(key_name));
		sprintf(key_name, "bets:%d:predictions", bet_no);
		if (NULL != iniparser_getstring(ini, key_name, NULL)) {
			cJSON_AddStringToObject(bet, "predictions", iniparser_getstring(ini, key_name, NULL));
		} else {
			break;
		}
		memset(key_name, 0x00, sizeof(key_name));
		sprintf(key_name, "bets:%d:range", bet_no);
		if (NULL != iniparser_getstring(ini, key_name, NULL)) {
			cJSON_AddStringToObject(bet, "range", iniparser_getstring(ini, key_name, NULL));
		} else {
			break;
		}
		cJSON_AddItemToArray(bets_info, bet);
		bet_no++;
	}
	cJSON_AddItemToObject(info, "bets_info", bets_info);
	dlg_info("\n%s", cJSON_Print(info));
	return retval;
}
