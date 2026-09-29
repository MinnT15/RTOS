/**
 * @file bt_18_3_simulated_cicd_pipeline_runner.c
 * @brief Trình mô phỏng điều phối luồng CI/CD Pipeline tự động (Automated Firmware CI/CD).
 * @author Embedded RTOS Senior Team
 * @date 2026-09-29
 *
 * @note Tiêu chuẩn MISRA C:2012 / Enterprise Embedded DevOps Pipeline.
 *       Mô phỏng 4 giai đoạn tự động trên máy chủ GitHub Actions:
 *       1. Static Analysis (Cppcheck/MISRA C)
 *       2. Unit Testing & Mocking (Ceedling/Unity)
 *       3. Linker Memory Footprint Check (.text, .rodata, .data, .bss against Flash/RAM budget)
 *       4. Firmware Packaging & Release.
 *       Áp dụng nguyên lý Fail-Fast: dừng pipeline ngay khi phát hiện vi phạm.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum {
    STAGE_STATUS_NOT_RUN = 0,
    STAGE_STATUS_PASS,
    STAGE_STATUS_FAIL
} StageStatus_t;

typedef struct {
    uint32_t maxFlashBytes;         /**< Ngân sách Flash tối đa của vi điều khiển (VD: 512 KB) */
    uint32_t maxRamBytes;           /**< Ngân sách RAM tối đa của vi điều khiển (VD: 128 KB) */
    uint32_t textBytes;             /**< Kích thước phân vùng .text */
    uint32_t rodataBytes;           /**< Kích thước phân vùng .rodata */
    uint32_t dataBytes;             /**< Kích thước phân vùng .data (chiếm cả Flash & RAM) */
    uint32_t bssBytes;              /**< Kích thước phân vùng .bss (chỉ chiếm RAM) */
} LinkerMapInfo_t;

typedef struct {
    uint32_t staticAnalysisViolations;
    uint32_t unitTestsPassed;
    uint32_t unitTestsTotal;
    LinkerMapInfo_t memory;
} ProjectBuildInput_t;

typedef struct {
    StageStatus_t staticStatus;
    StageStatus_t testStatus;
    StageStatus_t memoryStatus;
    StageStatus_t packageStatus;
    bool pipelinePassed;
    float flashUsagePercent;
    float ramUsagePercent;
} CIPipelineResult_t;

/**
 * @brief Stage 1: Kiểm tra phân tích tĩnh Static Analysis (Cppcheck / MISRA).
 */
StageStatus_t Stage_Static_Analysis(uint32_t violations) {
    return (violations == 0U) ? STAGE_STATUS_PASS : STAGE_STATUS_FAIL;
}

/**
 * @brief Stage 2: Kiểm tra Unit Tests trên Host PC.
 */
StageStatus_t Stage_Unit_Tests(uint32_t passed, uint32_t total) {
    if (total == 0U || passed < total) {
        return STAGE_STATUS_FAIL;
    }
    return STAGE_STATUS_PASS;
}

/**
 * @brief TODO: [x] Stage 3: Phân tích ngân sách bộ nhớ từ Linker Map.
 *        Flash Used = .text + .rodata + .data.
 *        RAM Used   = .data + .bss.
 *        Nếu Flash Used > maxFlashBytes hoặc RAM Used > maxRamBytes -> FAIL.
 * @param map Thông tin kích thước từ file .map.
 * @param outFlashPct Nhận % Flash đã dùng.
 * @param outRamPct Nhận % RAM đã dùng.
 * @return StageStatus_t
 */
StageStatus_t Stage_Memory_Footprint_Check(const LinkerMapInfo_t *map, float *outFlashPct, float *outRamPct) {
    if (map == NULL || outFlashPct == NULL || outRamPct == NULL) {
        return STAGE_STATUS_FAIL;
    }

    uint32_t flashUsed = map->textBytes + map->rodataBytes + map->dataBytes;
    uint32_t ramUsed   = map->dataBytes + map->bssBytes;

    *outFlashPct = ((float)flashUsed / (float)map->maxFlashBytes) * 100.0f;
    *outRamPct   = ((float)ramUsed   / (float)map->maxRamBytes)   * 100.0f;

    if ((flashUsed > map->maxFlashBytes) || (ramUsed > map->maxRamBytes)) {
        return STAGE_STATUS_FAIL; /* Phình bộ nhớ vượt trần vi điều khiển! */
    }

    return STAGE_STATUS_PASS;
}

/**
 * @brief Stage 4: Đóng gói Firmware Artifacts.
 */
StageStatus_t Stage_Package_Firmware(void) {
    return STAGE_STATUS_PASS;
}

/**
 * @brief TODO: [x] Bộ điều phối CI/CD Pipeline áp dụng nguyên tắc Fail-Fast.
 */
CIPipelineResult_t Run_Full_CI_Pipeline(const ProjectBuildInput_t *input) {
    CIPipelineResult_t res;
    memset(&res, 0, sizeof(CIPipelineResult_t));
    res.pipelinePassed = false;

    /* 1. Static Analysis */
    res.staticStatus = Stage_Static_Analysis(input->staticAnalysisViolations);
    if (res.staticStatus != STAGE_STATUS_PASS) {
        return res; /* Fail-Fast: Dừng pipeline ngay lập tức */
    }

    /* 2. Unit Tests */
    res.testStatus = Stage_Unit_Tests(input->unitTestsPassed, input->unitTestsTotal);
    if (res.testStatus != STAGE_STATUS_PASS) {
        return res;
    }

    /* 3. Memory Footprint */
    res.memoryStatus = Stage_Memory_Footprint_Check(&input->memory, &res.flashUsagePercent, &res.ramUsagePercent);
    if (res.memoryStatus != STAGE_STATUS_PASS) {
        return res;
    }

    /* 4. Packaging */
    res.packageStatus = Stage_Package_Firmware();
    if (res.packageStatus == STAGE_STATUS_PASS) {
        res.pipelinePassed = true;
    }

    return res;
}

int main(void) {
    printf("====================================================================\n");
    printf("     TEST HARNESS: Simulated CI/CD Pipeline Runner (MISRA C)        \n");
    printf("====================================================================\n");

    /* Kịch bản 1: Dự án chuẩn chỉnh vượt qua toàn bộ 4 giai đoạn CI */
    ProjectBuildInput_t goodProject = {
        .staticAnalysisViolations = 0U,
        .unitTestsPassed = 45U,
        .unitTestsTotal  = 45U,
        .memory = {
            .maxFlashBytes = 512U * 1024U, /* 512 KB Flash */
            .maxRamBytes   = 128U * 1024U, /* 128 KB RAM */
            .textBytes     = 180U * 1024U,
            .rodataBytes   = 30U * 1024U,
            .dataBytes     = 10U * 1024U,
            .bssBytes      = 50U * 1024U
        }
    };

    CIPipelineResult_t r1 = Run_Full_CI_Pipeline(&goodProject);
    assert(r1.pipelinePassed == true);
    assert(r1.staticStatus == STAGE_STATUS_PASS);
    assert(r1.testStatus == STAGE_STATUS_PASS);
    assert(r1.memoryStatus == STAGE_STATUS_PASS);
    assert(r1.packageStatus == STAGE_STATUS_PASS);
    /* Flash Used = 180 + 30 + 10 = 220 KB / 512 KB ~ 42.97% */
    /* RAM Used = 10 + 50 = 60 KB / 128 KB ~ 46.88% */
    printf("[PASS] Green Pipeline: All 4 stages passed! Flash: %.2f%%, RAM: %.2f%%\n",
           (double)r1.flashUsagePercent, (double)r1.ramUsagePercent);

    /* Kịch bản 2: Lập trình viên vô tình chèn mảng RAM tĩnh quá lớn làm tràn RAM */
    ProjectBuildInput_t badRamProject = goodProject;
    badRamProject.memory.bssBytes = 150U * 1024U; /* RAM Used = 10 + 150 = 160 KB > 128 KB */

    CIPipelineResult_t r2 = Run_Full_CI_Pipeline(&badRamProject);
    assert(r2.pipelinePassed == false);
    assert(r2.memoryStatus == STAGE_STATUS_FAIL);
    printf("[PASS] Fail-Fast trapped: Memory footprint exceeded 100%% of RAM budget (%.2f%%) -> Blocked PR!\n",
           (double)r2.ramUsagePercent);

    /* Kịch bản 3: Vi phạm phân tích tĩnh MISRA C */
    ProjectBuildInput_t badLintProject = goodProject;
    badLintProject.staticAnalysisViolations = 3U;

    CIPipelineResult_t r3 = Run_Full_CI_Pipeline(&badLintProject);
    assert(r3.pipelinePassed == false);
    assert(r3.staticStatus == STAGE_STATUS_FAIL);
    assert(r3.testStatus == STAGE_STATUS_NOT_RUN); /* Chưa từng được chạy do Fail-Fast */
    printf("[PASS] Fail-Fast trapped: 3 Static Analysis violations blocked pipeline at Stage 1!\n");

    printf("\n>>> [TEST PASSED] bt_18_3_simulated_cicd_pipeline_runner completed successfully.\n");
    return 0;
}
