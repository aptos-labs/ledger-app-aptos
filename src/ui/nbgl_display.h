#pragma once

#include "nbgl_use_case.h"

extern nbgl_contentTagValue_t pairs[6];
extern nbgl_contentTagValueList_t pair_list;

typedef struct use_case_review_ctx_s {
    nbgl_operationType_t operation_type;
    const nbgl_contentTagValueList_t *tag_value_list;
    const nbgl_icon_details_t *icon;
    const char *review_title;
    const char *review_sub_title;
    const char *finish_title;
    const nbgl_tipBox_t *tip_box;
    nbgl_choiceCallback_t choice_callback;
} use_case_review_ctx_t;

void nbgl_useCaseReviewVerify(nbgl_operationType_t operation_type,
                              const nbgl_contentTagValueList_t *tag_value_list,
                              const nbgl_icon_details_t *icon,
                              const char *review_title,
                              const char *review_sub_title,
                              const char *finish_title,
                              const nbgl_tipBox_t *tip_box,
                              nbgl_choiceCallback_t choice_callback);
