/**
 * @file bt_18_3_simulated_cicd_pipeline_runner.c
 * @brief Điều phối CI/CD Pipeline tự động (Static Analysis + Unit Test + Linker .map Check).
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
    uint32_t maxFlashBytes;
    uint32_t maxRamBytes;
    uint32_t textBytes;
    uint32_t rodataBytes;
    uint32_t dataBytes;
    uint32_t bssBytes;
} LinkerMapInfo_t;

typedef struct {
    const char     *branchName;
    uint32_t        staticAnalysisViolations;
    uint32_t        unitTestsPassed;
    uint32_t        unitTestsTotal;
    LinkerMapInfo_t memory;
} ProjectBuildInput_t;

typedef struct {
    StageStatus_t staticStatus;
    StageStatus_t testStatus;
    StageStatus_t memoryStatus;
    StageStatus_t packageStatus;
    bool          pipelinePassed;
    uint32_t      flashUsedBytes;
    uint32_t      ramUsedBytes;
    double        flashUsagePercent;
    double        ramUsagePercent;
} CIPipelineResult_t;

StageStatus_t Stage_Static_Analysis(uint32_t violations) {
    return (violations == 0U) ? STAGE_STATUS_PASS : STAGE_STATUS_FAIL;
}

StageStatus_t Stage_Unit_Tests(uint32_t passed, uint32_t total) {
    if ((total == 0U) || (passed < total)) {
        return STAGE_STATUS_FAIL;
    }
    return STAGE_STATUS_PASS;
}

/* TODO: [1] Tính toán Flash (.text + .rodata + .data) & RAM (.data + .bss) so với ngân sách MCU */
StageStatus_t Stage_Memory_Footprint_Check(const LinkerMapInfo_t *map,
                                           uint32_t *outFlashB, uint32_t *outRamB,
                                           double *outFlashPct, double *outRamPct) {
    if ((map == NULL) || (outFlashB == NULL) || (outRamB == NULL) ||
        (outFlashPct == NULL) || (outRamPct == NULL)) {
        return STAGE_STATUS_FAIL;
    }

    uint32_t flashUsed = map->textBytes + map->rodataBytes + map->dataBytes;
    uint32_t ramUsed   = map->dataBytes + map->bssBytes;

    *outFlashB   = flashUsed;
    *outRamB     = ramUsed;
    *outFlashPct = ((double)flashUsed / (double)map->maxFlashBytes) * 100.0;
    *outRamPct   = ((double)ramUsed   / (double)map->maxRamBytes)   * 100.0;

    if ((flashUsed > map->maxFlashBytes) || (ramUsed > map->maxRamBytes)) {
        return STAGE_STATUS_FAIL;
    }
    return STAGE_STATUS_PASS;
}

StageStatus_t Stage_Package_Firmware(void) {
    return STAGE_STATUS_PASS;
}

/* TODO: [2] Điều phối 4 Stage theo nguyên lý Fail-Fast của GitHub Actions */
CIPipelineResult_t Run_Full_CI_Pipeline(const ProjectBuildInput_t *input) {
    CIPipelineResult_t res;
    memset(&res, 0, sizeof(CIPipelineResult_t));
    res.pipelinePassed = false;

    res.staticStatus = Stage_Static_Analysis(input->staticAnalysisViolations);
    if (res.staticStatus != STAGE_STATUS_PASS) {
        return res;
    }

    res.testStatus = Stage_Unit_Tests(input->unitTestsPassed, input->unitTestsTotal);
    if (res.testStatus != STAGE_STATUS_PASS) {
        return res;
    }

    res.memoryStatus = Stage_Memory_Footprint_Check(&input->memory,
                                                    &res.flashUsedBytes, &res.ramUsedBytes,
                                                    &res.flashUsagePercent, &res.ramUsagePercent);
    if (res.memoryStatus != STAGE_STATUS_PASS) {
        return res;
    }

    res.packageStatus = Stage_Package_Firmware();
    if (res.packageStatus == STAGE_STATUS_PASS) {
        res.pipelinePassed = true;
    }
    return res;
}

static const char *prvStStr(StageStatus_t st) {
    if (st == STAGE_STATUS_PASS) { return "PASS   "; }
    if (st == STAGE_STATUS_FAIL) { return "FAIL!! "; }
    return "SKIPPED";
}

static void Print_CIPipelineSummary(const ProjectBuildInput_t *in, const CIPipelineResult_t *r) {
    printf("  [PR Branch: %-22s] => VERDICT: %s\n",
           in->branchName, r->pipelinePassed ? "GREEN (READY TO MERGE)" : "BLOCKED BY CI GATE!");
    printf("    Stage 1 (Cppcheck / MISRA C)   : [%s] (Violations=%u)\n",
           prvStStr(r->staticStatus), in->staticAnalysisViolations);
    printf("    Stage 2 (Unity/Ceedling Host)  : [%s] (%u/%u tests)\n",
           prvStStr(r->testStatus), in->unitTestsPassed, in->unitTestsTotal);
    printf("    Stage 3 (Linker .map Footprint): [%s] ", prvStStr(r->memoryStatus));
    if (r->memoryStatus != STAGE_STATUS_NOT_RUN) {
        printf("Flash=%uKB/%uKB (%.1f%%) | RAM=%uKB/%uKB (%.1f%%)\n",
               r->flashUsedBytes / 1024U, in->memory.maxFlashBytes / 1024U, r->flashUsagePercent,
               r->ramUsedBytes / 1024U,   in->memory.maxRamBytes / 1024U,   r->ramUsagePercent);
    } else {
        printf("(Không chạy do Fail-Fast ở Stage trước)\n");
    }
    printf("    Stage 4 (Release .bin/.elf)    : [%s]\n\n", prvStStr(r->packageStatus));
}

int main(void) {
    printf("================================================================================\n");
    printf("   BÀI TẬP 18.3: BỘ ĐIỀU PHỐI GITHUB ACTIONS CI/CD PIPELINE (FAIL-FAST GATE)    \n");
    printf("================================================================================\n");
    printf("Công thức Linker Map:\n");
    printf("  - Flash = .text + .rodata + .data  |  RAM = .data + .bss (+ Stack/Heap)\n\n");

    ProjectBuildInput_t goodProject = {
        "feat/motor-pid-v2",
        0U, 45U, 45U,
        { 512U * 1024U, 128U * 1024U, 180U * 1024U, 30U * 1024U, 10U * 1024U, 50U * 1024U }
    };
    CIPipelineResult_t r1 = Run_Full_CI_Pipeline(&goodProject);
    Print_CIPipelineSummary(&goodProject, &r1);
    assert(r1.pipelinePassed == true && r1.flashUsedBytes == 220U * 1024U && r1.ramUsedBytes == 60U * 1024U);

    ProjectBuildInput_t badRamProject = goodProject;
    badRamProject.branchName = "fix/huge-static-buf";
    badRamProject.memory.bssBytes = 150U * 1024U;
    CIPipelineResult_t r2 = Run_Full_CI_Pipeline(&badRamProject);
    Print_CIPipelineSummary(&badRamProject, &r2);
    assert(!r2.pipelinePassed && r2.memoryStatus == STAGE_STATUS_FAIL && r2.packageStatus == STAGE_STATUS_NOT_RUN);

    ProjectBuildInput_t badLintProject = goodProject;
    badLintProject.branchName = "wip/malloc-in-isr";
    badLintProject.staticAnalysisViolations = 3U;
    CIPipelineResult_t r3 = Run_Full_CI_Pipeline(&badLintProject);
    Print_CIPipelineSummary(&badLintProject, &r3);
    assert(!r3.pipelinePassed && r3.staticStatus == STAGE_STATUS_FAIL && r3.testStatus == STAGE_STATUS_NOT_RUN);

    printf(">>> [TEST PASSED] bt_18_3_simulated_cicd_pipeline_runner completed successfully.\n");
    return 0;
}
