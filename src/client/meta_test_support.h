/* Test seams for metaserver environment configuration. */

#ifndef META_TEST_SUPPORT_H
#define META_TEST_SUPPORT_H

/** Reset metaserver environment state to compiled defaults. */
void Meta_test_reset_environment(void);

/** Apply the current process environment once, as production does. */
void Meta_test_apply_environment(void);

/**
 * Read the effective endpoint selected for one metaserver.
 *
 * @param index Zero-based metaserver index.
 * @param name Receives a borrowed hostname.
 * @param address Receives a borrowed numeric address.
 * @param port Receives the shared program port.
 * @return Zero on success, or -1 for an invalid argument.
 */
int Meta_test_environment(int index, const char **name,
                          const char **address, int *port);

#endif /* META_TEST_SUPPORT_H */
