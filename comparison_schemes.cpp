#include "head.h"

//���������� Ȯ�� �� �ʿ䰡 ���� 20210515
//�� ��Ŵ���� �����Ǿ���.
short selected_set_in_comparison_schemes[CHANNEL_NUM + 1]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set
short** selected_ES_in_comparison_schemes;//[CHANNEL_NUM + 1][VERSION_NUM]; // �� ä�ο��� ����ϴ� ��Ʈ����Ʈ set�� ���ϴ� �� ������ � es���� ���õǾ��°�.
//�������� ������ Ʈ�����ڵ� ���ؼ� �迭 ũ�Ⱑ ������.
double remained_GHz_in_comparison_schemes[ES_NUM + 1];// processing capacity[es] - remained_GHz_in_comparison_schemes[es] �ϸ� used_GHz[es] ����. ��� ����� ���� GHz ����� ����.
double total_transfer_data_size_in_comparison_schemes[ES_NUM + 1];//�ǽð����� �����ϴ� ������ �������� �� ����� ����
//�̸��� �ٵ� �ʹ� �䵥 �̸� ���� ��� ��������...

void comparison_schemes(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	selected_ES_in_comparison_schemes = (short**)malloc(sizeof(short*) * (CHANNEL_NUM + 1));
	for (int ES = 1; ES <= ES_NUM; ES++) {
		remained_GHz_in_comparison_schemes[ES] = _server_list[ES].processing_capacity;
		total_transfer_data_size_in_comparison_schemes[ES] = 0;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		selected_ES_in_comparison_schemes[ch] = (short*)malloc(sizeof(short) * (_version_set->version_num));  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
		for (int ver = 1; ver < _version_set->version_num; ver++) {  // �������� ������ Ʈ�����ڵ� ���ϴϱ�
			selected_ES_in_comparison_schemes[ch][ver] = 0;
		}
		selected_set_in_comparison_schemes[ch] = 0;
		for (int version = 2; version <= _version_set->version_num - 1; version++) {
			selected_ES_in_comparison_schemes[ch][version] = 0;
		}
	}

	if (method_index == RR_AP) {
		method_RR_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RR_HPF) {
		method_RR_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RA_AP) {
		method_RD_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == RA_HPF) {
		method_RD_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == CA_AP) {
		method_CA_AP(_server_list, _channel_list, _version_set, cost_limit);
	}
	else if (method_index == CA_HPF) {
		method_CA_HPF(_server_list, _channel_list, _version_set, cost_limit);
	}

	for (int channel = 1; channel <= CHANNEL_NUM; channel++) {
		int set = 1;
		for (int version = 2; version <= _version_set->version_num - 1; version++) {
			if (selected_ES_in_comparison_schemes[channel][version] != 0)
				set += (_version_set->number_for_bit_opration >> (_version_set->set_versions_number_for_bit_opration - (version - 1)));
		}
		selected_set_in_comparison_schemes[channel] = set;
	}
	print_method(method_index, _server_list, _channel_list, _version_set);
}

void print_method(int method_index, server* _server_list, channel* _channel_list, bitrate_version_set* _version_set) {
	if (method_index == RR_AP) {
		printf("<<RR_AP>>\n");
	}
	else if (method_index == RR_HPF) {
		printf("<<RR_HPF>>\n");
	}
	else if (method_index == RA_AP) {
		printf("<<RA_AP>>\n");
	}
	else if (method_index == RA_HPF) {
		printf("<<RA_HPF>>\n");
	}
	else if (method_index == CA_AP) {
		printf("<<PA_AP>>\n");
	}
	else if (method_index == CA_HPF) {
		printf("<<PA_HPF>>\n");
	}
	double pwq_sum = 0;
	double vmaf_sum = 0;
	//double pwq_sum_range[10]; // 90~100%, 80~90% ... 0~10%������ Average PWQ�� ��.
	//double video_qualtiy_sum_range[10]; // 90~100%, 80~90% ... 0~10%������ Average video_qualtiy�� ��.
	/*for (int per_index = 0; per_index < 10; per_index++) {
		video_qualtiy_sum_range[per_index] = 0;
		pwq_sum_range[per_index] = 0;
	}*/

	int* number_of_transcoding = (int*)malloc(sizeof(int) * (_version_set->version_num));
	for (int version = 1; version <= _version_set->version_num - 1; version++) {
		number_of_transcoding[version] = 0;
	}
	// �� ��Ʈ����Ʈ ���� �� ���� transcoding�� ���� ��� ��. �� ������ range�� [0, CHANNEL_NUM]

	//printf("[ä��-��Ʈ]\n");
	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		/*if (version_set->pwq_set[ch][selected_set_other[ch]] < 0) {
			cout << "�����";
		}*/
		pwq_sum += _channel_list[ch].sum_of_pwq[selected_set_in_comparison_schemes[ch]];
		vmaf_sum += _channel_list[ch].sum_of_video_quality[selected_set_in_comparison_schemes[ch]];
		for (int version = 1; version <= _version_set->version_num - 1; version++) {
			if (((selected_set_in_comparison_schemes[ch] - 1) & _version_set->version_set_num >> ((_version_set->version_num - 2) - (version - 1)) || (version == 1))) {
				number_of_transcoding[version]++;
			}
		}
		//video_qualtiy_sum_range[(ch - 1) / (CHANNEL_NUM / 10)] += _channel_list[ch].video_quality_set[selected_set_in_comparison_schemes[ch]]; // 90~100%, 80~90% ... 0~10%������ Average VMAF�� ��.
		//pwq_sum_range[(ch - 1) / (CHANNEL_NUM / 10)] += _channel_list[ch].pwq_set[selected_set_in_comparison_schemes[ch]]; // 90~100%, 80~90% ... 0~10%������ Average VMAF�� ��.
	}
	printf("\n��ü pwq�� �� : %lf", pwq_sum);
	printf("\n��ü vmaf�� ��� : %lf\n\n", vmaf_sum / CHANNEL_NUM / _version_set->version_num);
	/*for (int per_index = 0; per_index < 10; per_index++) {
		printf("%d%~%d%% pwq�� ��/vmaf�� ��� : %lf, %lf\n", (per_index * 10), (per_index + 1) * 10, pwq_sum_range[per_index], video_qualtiy_sum_range[per_index] / (CHANNEL_NUM / 10) / version_set->version_num);
	}*/

	int total_cost = 0;
	double total_GHz = 0;
	for (int ES = 1; ES <= ES_NUM; ES++) {
		server* ES_ptr = &(_server_list[ES]);
		//printf("[�鿣�� ��� %d]\n", es);
		//printf("���� GHz, �ִ� GHz : %lf / %d\n", remained_GHz_in_comparison_schemes[es], get_backend_max_GHz(es));
		double used_GHz = _server_list[ES].processing_capacity - remained_GHz_in_comparison_schemes[ES];
		total_cost += calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES]/1024); // ��ü���� ���� �� ���� ����� GHz
		total_GHz += used_GHz;
	}
	printf("\n��� ��� : %d $\n", total_cost);
	printf("��� GHz : %lf GHz\n\n", total_GHz);

	/*for (int ver = _version_set->version_num - 1; ver >= 2; ver--) {
		printf("���� %d : %d\n", ver, number_of_transcoding[ver]);
	}*/
	printf("\n");
}

void method_RR_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
			if (ES > ES_NUM) {
				ES = 1;
			}

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) { // flag�� true�϶�, �� 1�� ������ �� Ʈ�����ڵ��ص� processing capacity�� ���� ���� ���, cost ������ �Ѵ��� Ȯ���Ѵ�.
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//es�� ch�� 1�� ���� Ʈ�����ڵ� �½�Ʈ�� �Ҵ��� ��, total_cost�� �Ѿ����� �� �Ѿ����� Ȯ��

			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� ��ü�� �����Ѵ�.2��~version_num-1�� ������ �Ҵ����� ����.
				//printf("1. RR_AP() : 1�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //�� ���� 2�� ����~ version_num-1�� ������ popularity ������ ���� �κ����� �Ҵ���.
		int ch = (*high_pop_channel_first.begin()).second; //���� ���� pop�� ä��
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // �� ���� �� ����

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
				if (ES > ES_NUM) {
					ES = 1;
				}

				if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int es = 1; es <= ES_NUM; es++) {
						if (is_full[es])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
						for (int v = (ver - 1); v >= 2; v--) {
							remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//es�� ch�� ver�� ���� Ʈ�����ڵ� �½�Ʈ�� �Ҵ��� ��, total_cost�� �Ѿ����� �� �Ѿ����� Ȯ��
				if (total_cost > cost_limit) {
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					for (int v = (ver - 1); v >= 2; v--) {
						remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}
//������� ����

void method_RR_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		bool is_full[ES_NUM + 1];
		for (int bk = 1; bk <= ES_NUM; bk++) {
			is_full[bk] = false;
		}
		do {
			ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
			if (ES > ES_NUM) {
				ES = 1;
			}

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//es�� ch�� 1�� ���� Ʈ�����ڵ� �½�Ʈ�� �Ҵ��� ��, total_cost�� �Ѿ����� �� �Ѿ����� Ȯ��

			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� ��ü�� �����Ѵ�.2��~version_num-1�� ������ �Ҵ����� ����.
				//printf("2. RR_HPF() : 2�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		high_pop_version_first.insert(make_pair(version_pop_average[ver], ver));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			bool is_full[ES_NUM + 1];
			for (int bk = 1; bk <= ES_NUM; bk++) {
				is_full[bk] = false;
			}
			do {
				ES++; //RR�� backend node �Ҵ��ϱ� ���� ++
				if (ES > ES_NUM) {
					ES = 1;
				}

				if ((remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver]) < 0) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver]) < 0);// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}


void method_RD_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int bk = 1; bk <= ES_NUM; bk++) {
		is_full[bk] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("3. RD_AP() : 3�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //�� ���� 2�� ����~ 6�� ������ pop ������ ���� �κ����� �Ҵ���.
		int ch = (*high_pop_channel_first.begin()).second; //���� ���� pop�� ä��
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // �� ���� �� ����

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

				if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
						for (int v = (ver - 1); v >= 2; v--) {
							remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					for (int v = (ver - 1); v >= 2; v--) {
						remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}

void method_RD_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	bool flag = true;
	int ES = 0; //0���� �����ϴ°��� 0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int full_node_cnt = 0;
		do {
			ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("4. RD_HPF() : 4�� �� ���ߴµ� �� ��\n");
				break;
			}

			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� �κ� ���·� �Ҵ���.
			selected_ES_in_comparison_schemes[ch][1] = ES;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int version = 2; version <= _version_set->version_num - 1; version++) {
		high_pop_version_first.insert(make_pair(version_pop_average[version], version));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				ES = rand() % ES_NUM + 1; //�������� �鿣�� ��� �Ҵ�

				if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int es = 1; es <= ES_NUM; es++) {
						if (is_full[es])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver]; //kbps�ϱ� GB ���� ��ȯ�� �ʿ��ϳ�...
				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // ������ ������ ���� �κ� ���·� �Ҵ���.
				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}


void method_CA_AP(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // ���� ��� ����� ���� ��� ����

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	server* ES_ptr = &(_server_list[ES]);
	set<pair<double, int>, greater<pair<double, int>>> high_pop_channel_first;

	bool is_full[ES_NUM + 1];
	for (int es = 1; es <= ES_NUM; es++) {
		is_full[es] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		do {
			if (!low_cost_first.size()) {
				flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ä���� ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
				break;
			}
			ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				low_cost_first.erase(*low_cost_first.begin());
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int bk = 1; bk <= ES_NUM; bk++) {
					if (is_full[bk])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("1. RR_AP() : 1�� �� ���ߴµ� �� ��\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU ������� ���� update�� �ϱ� ���� ����
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� ������ ���� ��� ���� �Ҵ���.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
		}

		high_pop_channel_first.insert(make_pair(_channel_list[ch].get_channel_popularity(), ch));
	}

	int high_channel_cnt = 0;
	while (high_pop_channel_first.size() && flag) { //�� ���� 2�� ����~ 6�� ������ pop ������ ���� �κ����� �Ҵ���.
		int ch = (*high_pop_channel_first.begin()).second; //���� ���� pop�� ä��
		high_pop_channel_first.erase(*high_pop_channel_first.begin()); // �� ���� �� ����

		for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ä���� ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
				ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���
				if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					low_cost_first.erase(*low_cost_first.begin());
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
						for (int v = (ver - 1); v >= 2; v--) {
							remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
							total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
							selected_ES_in_comparison_schemes[ch][v] = 0;
						}
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					//���� ver ������ ~ 2������ �Ҵ��� �� �����Ѵ�.
					for (int v = (ver - 1); v >= 2; v--) {
						remained_GHz_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] += _channel_list[ch].video_GHz[v];
						total_transfer_data_size_in_comparison_schemes[selected_ES_in_comparison_schemes[ch][v]] -= _version_set->data_size[v];
						selected_ES_in_comparison_schemes[ch][v] = 0;
					}

					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU ������� ���� update�� �ϱ� ���� ����

				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // �ش� ������ ���� ������ ���� ��� ���� �Ҵ���.
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES]) / 1024, ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}

void method_CA_HPF(server* _server_list, channel* _channel_list, bitrate_version_set* _version_set, int cost_limit) {
	set<pair<int, int>, less<pair<int, int>>> low_cost_first; // ���� ��� ������ ���� ��� ����

	for (int es = 1; es <= ES_NUM; es++) {
		server* es_ptr = &(_server_list[es]);
		low_cost_first.insert(make_pair(calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024), es));
	}

	bool flag = true;
	int ES = 0; //0�� �鿣�� ��� �ε����� �Ǵ°� �ƴ�
	server* ES_ptr = &(_server_list[ES]);
	int full_node_cnt = 0;

	bool is_full[ES_NUM + 1];
	for (int bk = 1; bk <= ES_NUM; bk++) {
		is_full[bk] = false;
	}

	for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
		int full_node_cnt = 0;
		do {
			if (!low_cost_first.size()) {
				flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
				break;
			}
			ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���

			if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0)) {
				low_cost_first.erase(*low_cost_first.begin());
				if (!is_full[ES]) {
					is_full[ES] = true;
				}
				int full_node_cnt = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					if (is_full[es])
						full_node_cnt++;
				}
				if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
			}
		} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[1] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

		if (flag) {
			double total_cost = 0;
			for (int es = 1; es <= ES_NUM; es++) {
				server* es_ptr = &(_server_list[es]);
				if (ES == es)
					total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[1]) / 1024);
				else
					total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
			}//������ �� á���� �ƴ��� Ȯ��
			if (total_cost > cost_limit) {
				flag = false; //���� �Ҵ� �����Ѵ�.
				//printf("1. RR_AP() : 1�� �� ���ߴµ� �� ��\n");
				break;
			}

			low_cost_first.erase(*low_cost_first.begin()); //CPU ������� ���� update�� �ϱ� ���� ����
			total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[1];
			remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[1]; // 1�� ������ ���� ������ ���� ��� ���� �Ҵ���.
			low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

			selected_ES_in_comparison_schemes[ch][1] = ES;
		}
	}

	set<pair<double, int>, greater<pair<double, int>> > high_pop_version_first;  //���� ��� pop�� ���� ������ ã�� ����
	double* version_pop_average = (double*)malloc(sizeof(double) * (_version_set->version_num)); // ���� 1�� �̹� Ʈ�����ڵ��߰� ������������ �����̶� Ʈ�����ڵ� ����
	for (int ver = 2; ver <= _version_set->version_num - 1; ver++) {
		version_pop_average[ver] = 0;
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			version_pop_average[ver] += _channel_list[ch].popularity[ver];
		}
		version_pop_average[ver] /= CHANNEL_NUM;
	}
	for (int version = 2; version <= _version_set->version_num - 1; version++) {
		high_pop_version_first.insert(make_pair(version_pop_average[version], version));
	}
	//������� ����

	while (high_pop_version_first.size() && flag) {
		int ver = (*high_pop_version_first.begin()).second; // ���� ���� pop�� ����
		high_pop_version_first.erase(*high_pop_version_first.begin()); // �� ���� �� ����
		for (int ch = 1; ch <= CHANNEL_NUM; ch++) {
			do {
				if (!low_cost_first.size()) {
					flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
					break;
				}
				ES = (*low_cost_first.begin()).second; // ���� �Ŀ��� ���� ���
				if (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0)) {
					low_cost_first.erase(*low_cost_first.begin());
					if (!is_full[ES]) {
						is_full[ES] = true;
					}
					int full_node_cnt = 0;
					for (int bk = 1; bk <= ES_NUM; bk++) {
						if (is_full[bk])
							full_node_cnt++;
					}
					if (full_node_cnt >= ES_NUM) { // ��� �鿣�尡 �� ���ִٸ�
						flag = false; //���� �Ҵ� �����Ѵ�. �̰ͺ��� �� ���� �α⵵�� ������ ���� �� ������ �ĵ�, �׵��� ����� �ʿ�� ����.
						break;
					}
				}
			} while (is_full[ES] || (remained_GHz_in_comparison_schemes[ES] - _channel_list[ch].video_GHz[ver] < 0));// �ش� ��尡 �� á���� �ƴ��� Ȯ��

			if (flag) {
				double total_cost = 0;
				for (int es = 1; es <= ES_NUM; es++) {
					server* es_ptr = &(_server_list[es]);
					if (ES == es)
						total_cost += calculate_ES_cost(es_ptr, (total_transfer_data_size_in_comparison_schemes[es] + _version_set->data_size[ver]) / 1024);
					else
						total_cost += calculate_ES_cost(es_ptr, total_transfer_data_size_in_comparison_schemes[es] / 1024);
				}//������ �� á���� �ƴ��� Ȯ��
				if (total_cost > cost_limit) {
					flag = false; //���� �Ҵ� �����Ѵ�.
					break;
				}

				low_cost_first.erase(*low_cost_first.begin());//CPU ������� ���� update�� �ϱ� ���� ����
				total_transfer_data_size_in_comparison_schemes[ES] += _version_set->data_size[ver];
				remained_GHz_in_comparison_schemes[ES] -= _channel_list[ch].video_GHz[ver]; // �ش� ������ ���� ������ ���� ��� ���� �Ҵ���.
				low_cost_first.insert(make_pair(calculate_ES_cost(ES_ptr, total_transfer_data_size_in_comparison_schemes[ES] / 1024), ES));

				selected_ES_in_comparison_schemes[ch][ver] = ES;
			}
		}
	}
}