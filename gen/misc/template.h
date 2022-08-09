#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

/**
 * Load the template file.
 * \param filename The template filename.
 */
extern void
template_load (const char *filename);

/**
 * Lookup the entry from the template file.
 * \param name The entry's name.
 * \return The entry's value.
 */
extern const char*
template_lookup (const char *name);

/**
 * Clear the loaded template entries.
 */
extern void
template_clear (void);

#endif /*_TEMPLATE_H_*/