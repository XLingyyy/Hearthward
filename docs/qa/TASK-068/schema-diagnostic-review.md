# Actual Schema diagnostic

The root ran the unchanged frozen cases C01,C08,C09,C18,C19,C27,C40,A01,U01 on the real Vulkan runtime with the existing System and compressed complete catalog. The only production difference from the prior 60-expression candidate was Schema limits grammar matching ValidLimit and each capability's constraints.

Raw understanding: 3/9; clear end-to-end: 2/7; execution: 1/6; extra items: 0. This diagnostic subset does not meet or replace the unchanged 60-expression gate; deterministic boundaries were not repeated. Runtime closed successfully.

- C40 now returned cancel with empty limits and cancelled the actual active command.
- C08 retained max:wood:3 but invented once:arrow. The grammar accepts the format; the existing validator correctly rejected the unsupported meaning.
- C09 still selected bag despite explicit camp materials; C18/C19 selected incorrect transport direction; C27 selected medicine_half for herb; A01 invented quantity four.
- C01 stayed generation1/full_relevant with input3190, the same token count as the previous candidate; the stricter Schema did not add input tokens in this real request.

The schema fixes invalid format freedom and leaves semantic errors visible. Parse/Validate, authority, confirmation, locked model/budgets, frozen expressions and expected values remain unchanged. No overall TASK-068 or TASK-072 PASS is claimed.
