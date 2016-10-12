/*
 * $HEADER$
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

extern const scon_mca_base_component_t scon_mca_common_orte_component;

const scon_mca_base_component_t *mca_common_base_static_components[] = {
  &scon_mca_common_orte_component,
  NULL
};

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
