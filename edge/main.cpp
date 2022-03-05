#include "head.h"
int main() {
	bool bandwidth_apply_flag = true;
	srand(SEED);

	double ratio = 0.5;// 2 3, 4, 5, 6
	int pop_type = MVP;
	int metric_type = VMAF;
	//int metric_type = PSNR;
	int model = LINEAR_MODEL;
	//int model = ONOFF_MODEL;
	int bitrate_set = 0; // 0~4 (디폴트 0)
	//이 위의 인자들을 실험 환경에 따라 변경

	server server_list[NUM_OF_ES + 1];
	channel channel_list[NUM_OF_CHANNEL + 1];
	bitrate_version_set version_set(bitrate_set, metric_type); // default 0, VMAF;
	channel_initialization(channel_list, &version_set, pop_type, metric_type);
	server_initalization(server_list, model, bandwidth_apply_flag);
	set_coverage_infomation(channel_list, server_list);
	server_initalization_for_bandwidth(server_list, model, bandwidth_apply_flag);


	double cost_limit = get_total_charge(server_list, model) * ratio;
	printf("===== START =====\n");
	printf("비용 한도 : %lf\n", cost_limit);
	printf("감마 분포 - k 값 : %lf, 세타 값 : %lf\n\n", K_gamma, THETA_gamma);
	printf("엣지 수 : %d, 채널 수 : %d\n\n", NUM_OF_ES, NUM_OF_CHANNEL);

	set<double, greater<double>> ver_GHz;
	set<double, greater<double>> ver_Mbps;
	set<double, greater<double>> ES_processing_capacity;
	set<double, greater<double>> ES_maximum_bandwidth;
	for (int ch = 1; ch <= NUM_OF_CHANNEL; ch++) {
		for (int ver = 1; ver <= version_set.version_num - 1; ver++) {
			ver_GHz.insert(channel_list[ch].video_GHz[ver]);
			ver_Mbps.insert(channel_list[ch].video_Mbps[ver]);
		}
	}
	for (int es = 1; es <= NUM_OF_ES; es++) {
		ES_processing_capacity.insert(server_list[es].processing_capacity);
		ES_maximum_bandwidth.insert(server_list[es].maximum_bandwidth);
	}

	double ver_max_GHz = (*ver_GHz.begin());
	double ver_max_Mbps = (*ver_Mbps.begin());
	double ver_max_processing_capacity = (*ES_processing_capacity.begin());
	double ver_max_bandwidth = (*ES_maximum_bandwidth.begin());

	pair<pair<double, double>, pair<double, double>> nomalized_base_value = make_pair(make_pair(ver_max_GHz, ver_max_Mbps), make_pair(ver_max_processing_capacity, ver_max_bandwidth));
	clock_t start, end, spent_time;
	start = clock();
	algorithm_run(server_list, channel_list, &version_set, cost_limit, model, bandwidth_apply_flag, nomalized_base_value);
	end = clock();
	spent_time = end - start;

	//printf("===== 비교 스킴 =====\n\n");

	comparison_schemes(RD_AP, server_list, channel_list, &version_set, cost_limit, model);
	comparison_schemes(RD_HPF, server_list, channel_list, &version_set, cost_limit, model);

	comparison_schemes(GHz_WF_AP, server_list, channel_list, &version_set, cost_limit, model);
	comparison_schemes(GHz_WF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	//if (model == CPU_USAGE_MODEL || model == STEP_MODEL) {
	if (model == LINEAR_MODEL) {
		comparison_schemes(cost_WF_AP, server_list, channel_list, &version_set, cost_limit, model);
		comparison_schemes(cost_WF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	}
	if (model == ONOFF_MODEL) {
		comparison_schemes(LPF_AP, server_list, channel_list, &version_set, cost_limit, model);
		comparison_schemes(LPF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	}

	if (bandwidth_apply_flag) {
		comparison_schemes(Mbps_WF_AP, server_list, channel_list, &version_set, cost_limit, model);
		comparison_schemes(Mbps_WF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	}

	printf("\n%lf second\n\n", (double)spent_time / CLOCKS_PER_SEC);
	printf("===== FINISH =====\n");
}
