#include "head.h"
int main() {
	srand(SEED);

	double ratio = 0.25; // �ּ� 0.25�� ���� 0.25 0.5 0.75 1
	int pop_type = MVP;
	int metric_type = VMAF;
	//int metric_type = SSIM;
	int model = CPU_USAGE_MODEL;
	//int model = ONOFF_MODEL;
	int bitrate_set = 0;
	//�� ���� ���ڵ��� ���� ȯ�濡 ���� ����

	server server_list[NUM_OF_ES + 1];
	channel channel_list[NUM_OF_CHANNEL + 1];
	bitrate_version_set version_set(bitrate_set, metric_type); // default 0, VMAF;
	channel_initialization(channel_list, &version_set, pop_type, metric_type);
	server_initalization(server_list, model);
	set_coverage_infomation(channel_list, server_list);

	double cost_limit = get_total_charge(server_list, model) * ratio;
	printf("===== START =====\n");
	printf("��� �ѵ� : %lf\n", cost_limit);
	printf("���� ���� - k �� : %lf, ��Ÿ �� : %lf\n\n", K_gamma, THETA_gamma);
	printf("���� �� : %d, ä�� �� : %d\n\n", NUM_OF_ES, NUM_OF_CHANNEL);

	clock_t start, end, spent_time;
	start = clock();
	algorithm_run(server_list, channel_list, &version_set, cost_limit, model);
	end = clock();
	spent_time = end - start;
	printf("%lf second\n\n", (double)spent_time / CLOCKS_PER_SEC);

	//�񱳽�Ŵ ���� �ڵ� �� ��

	printf("===== �� ��Ŵ =====\n\n");

	comparison_schemes(GHz_WF_AP, server_list, channel_list, &version_set, cost_limit, model);
	comparison_schemes(GHz_WF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	//if (model == CPU_USAGE_MODEL || model == STEP_MODEL) {
	if (model == CPU_USAGE_MODEL) {
		comparison_schemes(cost_WF_AP, server_list, channel_list, &version_set, cost_limit, model);
		comparison_schemes(cost_WF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	}
	if (model == ONOFF_MODEL) {
		comparison_schemes(LPF_AP, server_list, channel_list, &version_set, cost_limit, model);
		comparison_schemes(LPF_HPF, server_list, channel_list, &version_set, cost_limit, model);
	}
	comparison_schemes(RD_AP, server_list, channel_list, &version_set, cost_limit, model);
	comparison_schemes(RD_HPF, server_list, channel_list, &version_set, cost_limit, model);

	printf("===== FINISH =====\n");
}