#include "head.h"

//���������� Ȯ�� �� �ʿ䰡 ���� 20210515
//�� ��Ŵ���� �����Ǿ���.
short selected_set_in_comparison_schemes[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
short** selected_ES_in_comparison_schemes;//[CHANNEL_NUM + 1][VERSION_NUM]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set�� ���ϴ� �� ������ � es���� ���õǾ��°�.
//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.
double used_GHz_in_comparison_schemes[ES_NUM + 1];
double total_transfer_data_size_in_comparison_schemes[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����

//short ES_count_in_comparison_schemes[ES_NUM + 1];
short** ES_version_count_in_comparison_schemes;

void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	ES_version_count_in_comparison_schemes = (short**)malloc(sizeof(short*) * (ES_NUM + 1));
	for (int ES = 0; ES <= ES_NUM; ES++) {
		used_GHz_in_comparison_schemes[ES] = 0;
		total_transfer_data_size_in_comparison_schemes[ES] = 0;

		ES_version_count_in_comparison_schemes[ES] = (short*)malloc(sizeof(short) * (_version_set->version_num));
		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			ES_version_count_in_comparison_schemes[ES][ver] = 0;
		}
	}

	selected_ES_in_comparison_schemes = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		selected_ES_in_comparison_schemes[ch] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES_in_comparison_schemes[ch][ver] = -1;
		}
	}

	if (method_index == GHz_worst_fit_HPF) {
		GHz_worst_fit_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}

	set_version_set(_version_set, selected_set_in_comparison_schemes, selected_ES_in_comparison_schemes);
	print_method(method_index, _server_list, _channel_list, _version_set);
}

void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set) {
	if (method_index == GHz_worst_fit_HPF) {
		printf("<<lowest_number_of_allocated_version_HPF>>\n");
	}
	
	double total_GHz = 0;
	double total_pwq = 0;
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		total_GHz += _channel_list[ch].sum_of_version_set_GHz[selected_set_in_comparison_schemes[ch]];
		total_pwq += _channel_list[ch].sum_of_pwq[selected_set_in_comparison_schemes[ch]];
	}

	double total_cost = 0; 
	double remained_GHz[ES_NUM + 1]; // processing capacity[es] - used_GHz[es] �ϸ� remained_GHz[es] �ϸ� ����. ��� ����� ���� GHz ����� ����.

	for (int ES = 0; ES <= ES_NUM; ES++) {
		short ES_total_count = get_ES_total_count(ES, _version_set);

		if (ES_total_count > 0) {
			total_cost += calculate_ES_cost(&(_server_list[ES]), total_transfer_data_size_in_comparison_schemes[ES] / 1024);
			remained_GHz[ES] = _server_list[ES].processing_capacity - used_GHz_in_comparison_schemes[ES];
		}
	}
	std::printf(" total_GHz : %lf GHz, total_pwq : %lf, total_cost : %lf\n", total_GHz, total_pwq, total_cost);
}

void GHz_worst_fit_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	//���� ���� - �� ES server�� coverage�� Ȯ���ϰ�, ����� GHz�� ���� ���� ES�� �Ҵ��Ѵ�. 
	//���� ���� - ���� �α⵵�� ���� ä��-������ �켱������ �����Ͽ� ES�� (������ ������ ��) �Ҵ��Ѵ�.

	set<pair<double, pair<int, int>>, greater<pair<double, pair<int, int>>> > version_popularities_set;
	
	//ó���� 1�� ������ ���ؼ��� set�� �����Ѵ�.
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		version_popularities_set.insert(make_pair(_channel_list[ch].popularity[1], make_pair(ch, 1)));
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		//���� ��� ä���� 1�� ������ �Ҵ��Ѵ�.
		int ch = (*version_popularities_set.begin()).second.first;
		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, ����� GHz�� ���� �������� ����. 
		set<pair<double, int>> lowest_used_GHz_of_ES;
		for (int ES = 1; ES <= ES_NUM; ES++) {
			if(_channel_list[ch].available_server_list[ES])
				lowest_used_GHz_of_ES.insert(make_pair(used_GHz_in_comparison_schemes[ES], ES));
		}

		while (!lowest_used_GHz_of_ES.empty()) {
			int ES = (*lowest_used_GHz_of_ES.begin()).second;
			lowest_used_GHz_of_ES.erase(lowest_used_GHz_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - used_GHz_in_comparison_schemes[ES];
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else{
					if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[1] >= 0) && (total_cost <= cost_limit)) {
				used_GHz_in_comparison_schemes[ES] += _channel_list[ch].video_GHz[1];
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];

				selected_ES_in_comparison_schemes[ch][1] = ES;
				ES_version_count_in_comparison_schemes[ES][1]++;

				is_allocated = true;
				break;
			} //������ �� ���� ��� �Ҵ�.
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][1], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - used_GHz_in_comparison_schemes[0];
			if (GHz - _channel_list[ch].video_GHz[1] >= 0) {
				used_GHz_in_comparison_schemes[0] += _channel_list[ch].video_GHz[1];
				total_transfer_data_size_in_comparison_schemes[0] += _version_set->data_size[1];

				selected_ES_in_comparison_schemes[ch][1] = 0;
				ES_version_count_in_comparison_schemes[0][1]++;
			}
		}
	}

	//��� ä���� 2~N^ver-1 �����鿡 ���� �Ҵ��� �����Ѵ�.
	version_popularities_set.clear();
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			version_popularities_set.insert(make_pair(_channel_list[ch].popularity[ver], make_pair(ch, ver)));
		}
	}

	while (!version_popularities_set.empty()) {
		bool is_allocated = false;
		int ch = (*version_popularities_set.begin()).second.first;
		int ver = (*version_popularities_set.begin()).second.second;

		version_popularities_set.erase(version_popularities_set.begin());
		//���� �α⸹�� ch�� ��.

		//���� �� ä���� Ŀ������ ���� ES�� ã��, �� ES�� �Ҵ�� ver�� ������ ������ ���� �������� ����. 
		set<pair<int, int>> number_of_allocated_versions_of_ES;
		for (int ES = 1; ES <= ES_NUM; ES++) {
			if (_channel_list[ch].available_server_list[ES])
				number_of_allocated_versions_of_ES.insert(make_pair(used_GHz_in_comparison_schemes[ES], ES));
		}

		while (!number_of_allocated_versions_of_ES.empty()) {
			int ES = (*number_of_allocated_versions_of_ES.begin()).second;
			number_of_allocated_versions_of_ES.erase(number_of_allocated_versions_of_ES.begin());

			double GHz = _server_list[ES].processing_capacity - used_GHz_in_comparison_schemes[ES];
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				if (es == ES)
					total_cost += calculate_ES_cost(&(_server_list[es]), (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
				else {
					if (get_ES_total_count(es, _version_set))
						total_cost += calculate_ES_cost(&(_server_list[es]), total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}
			}

			if ((GHz - _channel_list[ch].video_GHz[ver] >= 0) && (total_cost <= cost_limit)) {
				used_GHz_in_comparison_schemes[ES] += _channel_list[ch].video_GHz[ver];
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];

				selected_ES_in_comparison_schemes[ch][ver] = ES;
				ES_version_count_in_comparison_schemes[ES][ver]++;

				is_allocated = true;
				break;
			}
			//number_of_allocated_versions_of_ES.insert(make_pair(ES_version_count_in_comparison_schemes[ch][ver], ES));
		}

		if (!is_allocated) { //��� ������ �Ҵ��� �Ұ����� ������
			double GHz = _server_list[0].processing_capacity - used_GHz_in_comparison_schemes[0];
			if (GHz - _channel_list[ch].video_GHz[ver] >= 0) {
				used_GHz_in_comparison_schemes[0] += _channel_list[ch].video_GHz[ver];
				total_transfer_data_size_in_comparison_schemes[0] += _version_set->data_size[ver];

				selected_ES_in_comparison_schemes[ch][ver] = 0;
				ES_version_count_in_comparison_schemes[0][ver]++;

				break;
			}
		}
	}
}
//������� ����

short get_ES_total_count(int ES, bitrate_version_set* _version_set) {
	short ES_total_count = 0;
	for (int ver = 1; ver <= _version_set->version_num - 1; ver++) {
		ES_total_count += ES_version_count_in_comparison_schemes[ES][ver];
	}
	return ES_total_count;
}