subdirs-y = $(filter-out sub.mk,$(notdir $(wildcard $(sub-dir)/*)))
