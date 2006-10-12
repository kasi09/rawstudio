/*
 * Copyright (C) 2006 Anders Brander <anders@brander.dk> and 
 * Anders Kvist <akv@lnxbx.dk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include "color.h"
#include "matrix.h"
#include "rs-batch.h"
#include "rawstudio.h"
#include "conf_interface.h"
#include "gtk-interface.h"
#include "filename.h"
#include <gettext.h>
#include <lcms.h>

void gui_cms_in_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data);
void gui_cms_di_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data);
void gui_cms_ex_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data);
void gui_cms_intent_combobox_changed(GtkComboBox *combobox, gpointer user_data);
void gui_cms_in_profile_button_clicked(GtkButton *button, gpointer user_data);
void gui_cms_di_profile_button_clicked(GtkButton *button, gpointer user_data);
void gui_cms_ex_profile_button_clicked(GtkButton *button, gpointer user_data);

gchar *color_profiles[] = {
	"*.icc", 
	"*.icm", 
	"*.ICC", 
	"*.ICM", 
	NULL
};

const RS_FILETYPE *
gui_filetype_combobox_get_filetype(GtkComboBox *widget)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	RS_FILETYPE *filetype;

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter);
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
	gtk_tree_model_get(model, &iter, 1, &filetype, -1);
	return(filetype);
}

const gchar *
gui_filetype_combobox_get_ext(GtkComboBox *widget)
{
	const RS_FILETYPE *filetype = gui_filetype_combobox_get_filetype(widget);
	return(filetype->ext);
}

GtkWidget *
gui_filetype_combobox()
{
	extern RS_FILETYPE *filetypes;
	RS_FILETYPE *filetype = filetypes;

	GtkListStore *model;
	GtkComboBox *combo;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;

	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

	while(filetype)
	{
		if (filetype->save)
		{
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter, 0, filetype->description, 1, filetype, -1);
		}
		filetype = filetype->next;
	}

	combo = GTK_COMBO_BOX(gtk_combo_box_new());

	gtk_combo_box_set_model (combo, GTK_TREE_MODEL (model));
	gtk_combo_box_set_active (combo, 0);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", 0, NULL);

	return(GTK_WIDGET(combo));
}

void
checkbox_set_conf(GtkToggleButton *togglebutton, gpointer user_data)
{
	const gchar *path = user_data;
	rs_conf_set_boolean(path, togglebutton->active);
	return;
}

GtkWidget *
checkbox_from_conf(const gchar *conf, gchar *label, gboolean default_value)
{
	gboolean check = default_value;
	GtkWidget *checkbox;
	rs_conf_get_boolean(conf, &check);
	checkbox = gtk_check_button_new_with_label(label);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox),
		check);
	g_signal_connect ((gpointer) checkbox, "toggled",
		G_CALLBACK (checkbox_set_conf), (gpointer) conf);
	return(checkbox);
}

GtkWidget *gui_tooltip_no_window(GtkWidget *widget, gchar *tip_tip, gchar *tip_private)
{
	GtkWidget *e;
	GtkTooltips *tip;

	tip = gtk_tooltips_new();
	e = gtk_event_box_new();
	gtk_tooltips_set_tip(tip, e, tip_tip, tip_private);
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(e), widget);

	return e;
}

void gui_tooltip_window(GtkWidget *widget, gchar *tip_tip, gchar *tip_private)
{
	GtkTooltips *tip;

	tip = gtk_tooltips_new();
	gtk_tooltips_set_tip(tip, widget, tip_tip, tip_private);
	gtk_widget_show(widget);

	return;
}

void
gui_batch_directory_entry_changed(GtkEntry *entry, gpointer user_data)
{
	rs_conf_set_string(CONF_BATCH_DIRECTORY, gtk_entry_get_text(entry));
	return;
}

void
gui_batch_filename_entry_changed(GtkEntry *entry, gpointer user_data)
{
	rs_conf_set_string(CONF_BATCH_FILENAME, gtk_entry_get_text(entry));
	return;
}

void
gui_batch_filetype_entry_changed(GtkEntry *entry, gpointer user_data)
{
/*	rs_conf_set_string(CONF_BATCH_FILETYPE, gtk_entry_get_text(entry)); FIXME */
	return;
}

void
gui_export_changed_helper(GtkLabel *label)
{
	gchar *parsed = NULL;
	gchar *directory;
	gchar *filename;
	GString *final;
	RS_FILETYPE *filetype;

	directory = rs_conf_get_string(CONF_EXPORT_DIRECTORY);
	filename = rs_conf_get_string(CONF_EXPORT_FILENAME);
	rs_conf_get_filetype(CONF_EXPORT_FILETYPE, &filetype);

	parsed = filename_parse(filename, NULL);

	final = g_string_new("<small>");
	if (directory)
	{
		g_string_append(final, directory);
		g_free(directory);
	}
	g_string_append(final, parsed);
	g_free(parsed);
	g_string_append(final, ".");
	g_string_append(final, filetype->ext);
	g_string_append(final, "</small>");

	gtk_label_set_markup(label, final->str);

	g_string_free(final, TRUE);

	return;
}

void
gui_export_directory_entry_changed(GtkEntry *entry, gpointer user_data)
{
	GtkLabel *label = GTK_LABEL(user_data);
	rs_conf_set_string(CONF_EXPORT_DIRECTORY, gtk_entry_get_text(entry));

	gui_export_changed_helper(label);

	return;
}

void
gui_export_filename_entry_changed(GtkComboBox *combobox, gpointer user_data)
{
	GtkLabel *label = GTK_LABEL(user_data);
	rs_conf_set_string(CONF_EXPORT_FILENAME, gtk_combo_box_get_active_text(combobox));

	gui_export_changed_helper(label);

	return;
}

void
cms_enable_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	RS_BLOB *rs = (RS_BLOB *) user_data;
	rs_conf_set_boolean(CONF_CMS_ENABLED, togglebutton->active);
	rs->cms_enabled = togglebutton->active;
	rs_cms_prepare_transforms(rs);
	update_preview_callback(NULL, rs);
	return;
}

void
gui_cms_in_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data)
{
	RS_BLOB *rs = (RS_BLOB *) user_data;
	gchar *filename;

	rs_conf_set_integer(CONF_CMS_IN_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	filename = rs_get_profile(RS_CMS_PROFILE_IN);

	if (rs->loadProfile)
	{
		cmsCloseProfile(rs->loadProfile);
		rs->loadProfile = NULL;
	}

	if (filename)
	{
		rs->loadProfile = cmsOpenProfileFromFile(filename, "r");
		g_free(filename);
	}

	rs_cms_prepare_transforms(rs);
	update_preview_callback(NULL, rs);
	return;
}

void
gui_cms_di_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data)
{
	RS_BLOB *rs = (RS_BLOB *) user_data;
	gchar *filename;

	rs_conf_set_integer(CONF_CMS_DI_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	filename = rs_get_profile(RS_CMS_PROFILE_DISPLAY);

	if (rs->displayProfile)
	{
		cmsCloseProfile(rs->displayProfile);
		rs->displayProfile = NULL;
	}

	if (filename)
	{
		rs->displayProfile = cmsOpenProfileFromFile(filename, "r");
		g_free(filename);
	}

	rs_cms_prepare_transforms(rs);
	update_preview_callback(NULL, rs);
	return;
}

void
gui_cms_ex_profile_combobox_changed(GtkComboBox *combobox, gpointer user_data)
{
	RS_BLOB *rs = (RS_BLOB *) user_data;
	gchar *filename;

	rs_conf_set_integer(CONF_CMS_EX_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	filename = rs_get_profile(RS_CMS_PROFILE_EXPORT);

	if (rs->exportProfile)
	{
		cmsCloseProfile(rs->exportProfile);
		rs->exportProfile = NULL;
	}
	if (rs->exportProfileFilename)
	{
		g_free(rs->exportProfileFilename);
		rs->exportProfileFilename = NULL;
	}

	if (filename)
	{
		rs->exportProfile = cmsOpenProfileFromFile(filename, "r");
		if (rs->exportProfile)
			rs->exportProfileFilename = filename;
		else
			g_free(filename);
	}

	rs_cms_prepare_transforms(rs);
	update_preview_callback(NULL, rs);
	return;
}

void
gui_cms_intent_combobox_changed(GtkComboBox *combobox, gpointer user_data)
{
	RS_BLOB *rs = (RS_BLOB *) user_data;
	gint active = gtk_combo_box_get_active(combobox);
	rs_conf_set_cms_intent(CONF_CMS_INTENT, &active);
	rs->cms_intent = active;
	rs_cms_prepare_transforms(rs);
	update_preview_callback(NULL, rs);
	return;
}

gchar *
gui_cms_choose_profile(const gchar *path)
{
	gchar *ret = NULL;
	GtkWidget *fc;
	GtkFileFilter *file_filter_all;
	GtkFileFilter *file_filter_color_profiles;
	gint n;

	fc = gtk_file_chooser_dialog_new (_("Select color profile"), NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(fc), GTK_RESPONSE_ACCEPT);

	if (path)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fc), path);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fc), "/usr/share/color/icc");

	file_filter_all = gtk_file_filter_new();
	file_filter_color_profiles = gtk_file_filter_new();

	n=0;
	while(color_profiles[n])
	{
		gtk_file_filter_add_pattern(file_filter_color_profiles, color_profiles[n]);
		n++;
	}

	gtk_file_filter_add_pattern(file_filter_all, "*");

	gtk_file_filter_set_name(file_filter_all, _("All files"));
	gtk_file_filter_set_name(file_filter_color_profiles, _("Color profiles (icc and icm)"));

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), file_filter_color_profiles);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(fc), file_filter_all);
	if (gtk_dialog_run (GTK_DIALOG (fc)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fc));

		if (rs_cms_is_profile_valid(filename))
			ret = filename;
		else
		{
			GtkWidget *warning = gui_dialog_make_from_text(GTK_STOCK_DIALOG_WARNING,
				_("Not a valid color profile."), 
				_("The file you selected does not appear to be a valid color profile."));
			GtkWidget *ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
			gtk_dialog_add_action_widget(GTK_DIALOG(warning), ok_button, GTK_RESPONSE_ACCEPT);
			
			gtk_widget_show_all(warning);
			
        	gtk_dialog_run(GTK_DIALOG(warning));
			gtk_widget_destroy(warning);
		}
	}
	gtk_widget_destroy (fc);

	return(ret);
}

void
gui_cms_in_profile_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *combobox = GTK_WIDGET(user_data);
	gchar *filename;

	filename = gui_cms_choose_profile(NULL);

	if (filename)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), g_basename(filename));
		// FIXME: gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), __SELECT_LAST__);
		rs_conf_add_string_to_list_string(CONF_CMS_IN_PROFILE_LIST, filename);
		rs_conf_set_integer(CONF_CMS_IN_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	}
	return;
}

void
gui_cms_di_profile_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *combobox = GTK_WIDGET(user_data);
	gchar *filename; 

	filename = gui_cms_choose_profile(NULL);

	if (filename)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), g_basename(filename));
		// FIXME: gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), __SELECT_LAST__);
		rs_conf_add_string_to_list_string(CONF_CMS_DI_PROFILE_LIST, filename);
		rs_conf_set_integer(CONF_CMS_DI_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	}
	return;
}

void
gui_cms_ex_profile_button_clicked(GtkButton *button, gpointer user_data)
{
	GtkWidget *combobox = GTK_WIDGET(user_data);
	gchar *filename;

	filename = gui_cms_choose_profile(NULL);

	if (filename)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), g_basename(filename));
		// FIXME: gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), __SELECT_LAST__);
		rs_conf_add_string_to_list_string(CONF_CMS_EX_PROFILE_LIST, filename);
		rs_conf_set_integer(CONF_CMS_EX_PROFILE_SELECTED, gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
	}
	return;
}

GtkWidget *
gui_preferences_make_cms_page(RS_BLOB *rs)
{
	gint temp_conf_gint;
	GSList *temp_conf_gslist;
	GSList *temp_new_gslist = NULL;
	gchar *temp_conf_string;
	gboolean cms_enable;

	GtkWidget *cms_page;
	GtkWidget *cms_enable_check;
	GtkWidget *cms_in_profile_hbox;
	GtkWidget *cms_in_profile_label;
	GtkWidget *cms_in_profile_combobox;
	GtkWidget *cms_in_profile_button;
	GtkWidget *cms_di_profile_hbox;
	GtkWidget *cms_di_profile_label;
	GtkWidget *cms_di_profile_combobox;
	GtkWidget *cms_di_profile_button;
	GtkWidget *cms_ex_profile_hbox;
	GtkWidget *cms_ex_profile_label;
	GtkWidget *cms_ex_profile_combobox;
	GtkWidget *cms_ex_profile_button;
	GtkWidget *cms_intent_hbox;
	GtkWidget *cms_intent_label;
	GtkWidget *cms_intent_combobox;

	cms_page = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (cms_page), 6);

	cms_enable_check = gtk_check_button_new_with_label(_("Enable color management (experimental)"));
	if(!rs_conf_get_boolean(CONF_CMS_ENABLED, &cms_enable))
		cms_enable = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cms_enable_check), cms_enable);
	g_signal_connect ((gpointer) cms_enable_check, "toggled", G_CALLBACK (cms_enable_toggled), rs);
	gtk_box_pack_start (GTK_BOX (cms_page), cms_enable_check, FALSE, TRUE, 0);

	cms_in_profile_hbox = gtk_hbox_new(FALSE, 0);
	cms_in_profile_label = gtk_label_new(_("Input profile"));
	gtk_misc_set_alignment(GTK_MISC(cms_in_profile_label), 0.0, 0.5);
	cms_in_profile_combobox = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_in_profile_combobox), _("BuiltInRGBProfile"));
	temp_conf_gslist = rs_conf_get_list_string(CONF_CMS_IN_PROFILE_LIST);
	temp_conf_gint = 0;
	rs_conf_get_integer(CONF_CMS_IN_PROFILE_SELECTED, &temp_conf_gint);

	while(temp_conf_gslist)
	{
		temp_conf_string = (gchar *) temp_conf_gslist->data;
		if (g_file_test(temp_conf_string, G_FILE_TEST_EXISTS))
		{
			cmsHPROFILE color_profile = cmsOpenProfileFromFile(temp_conf_string, "r");
			if (color_profile)
			{
				cmsCloseProfile(color_profile);	
				gtk_combo_box_append_text(GTK_COMBO_BOX(cms_in_profile_combobox), g_basename(temp_conf_string));
				temp_new_gslist = g_slist_append(temp_new_gslist, (gpointer *) temp_conf_string);
			}
		}
		else
			temp_conf_gint = 0;
		temp_conf_gslist = temp_conf_gslist->next;
	}

	rs_conf_set_list_string(CONF_CMS_IN_PROFILE_LIST, temp_new_gslist);
	g_slist_free(temp_new_gslist);
	temp_new_gslist = NULL;
	if (temp_conf_gint < 0)
		temp_conf_gint = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(cms_in_profile_combobox), temp_conf_gint);
	cms_in_profile_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start (GTK_BOX (cms_in_profile_hbox), cms_in_profile_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_in_profile_hbox), cms_in_profile_combobox, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_in_profile_hbox), cms_in_profile_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_page), cms_in_profile_hbox, FALSE, TRUE, 0);

	cms_di_profile_hbox = gtk_hbox_new(FALSE, 0);
	cms_di_profile_label = gtk_label_new(_("Display profile"));
	gtk_misc_set_alignment(GTK_MISC(cms_di_profile_label), 0.0, 0.5);
	cms_di_profile_combobox = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_di_profile_combobox), _("sRGB"));
	temp_conf_gslist = rs_conf_get_list_string(CONF_CMS_DI_PROFILE_LIST);
	temp_conf_gint = 0;
	rs_conf_get_integer(CONF_CMS_DI_PROFILE_SELECTED, &temp_conf_gint);

	while(temp_conf_gslist)
	{
		temp_conf_string = (gchar *) temp_conf_gslist->data;
		if (g_file_test(temp_conf_string, G_FILE_TEST_EXISTS))
		{
			cmsHPROFILE color_profile = cmsOpenProfileFromFile(temp_conf_string, "r");
			if (color_profile)
			{
				cmsCloseProfile(color_profile);	
				gtk_combo_box_append_text(GTK_COMBO_BOX(cms_di_profile_combobox), g_basename(temp_conf_string));
				temp_new_gslist = g_slist_append(temp_new_gslist, (gpointer *) temp_conf_string);
			}
		}
		else
			temp_conf_gint = 0;
		temp_conf_gslist = temp_conf_gslist->next;
	}

	rs_conf_set_list_string(CONF_CMS_DI_PROFILE_LIST, temp_new_gslist);
	g_slist_free(temp_new_gslist);
	temp_new_gslist = NULL;
	if (temp_conf_gint < 0)
		temp_conf_gint = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(cms_di_profile_combobox), temp_conf_gint);
	cms_di_profile_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start (GTK_BOX (cms_di_profile_hbox), cms_di_profile_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_di_profile_hbox), cms_di_profile_combobox, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_di_profile_hbox), cms_di_profile_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_page), cms_di_profile_hbox, FALSE, TRUE, 0);

	cms_ex_profile_hbox = gtk_hbox_new(FALSE, 0);
	cms_ex_profile_label = gtk_label_new(_("Export profile"));
	gtk_misc_set_alignment(GTK_MISC(cms_ex_profile_label), 0.0, 0.5);
	cms_ex_profile_combobox = gtk_combo_box_entry_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_ex_profile_combobox), _("sRGB"));
	temp_conf_gslist = rs_conf_get_list_string(CONF_CMS_EX_PROFILE_LIST);
	temp_conf_gint = 0;
	rs_conf_get_integer(CONF_CMS_EX_PROFILE_SELECTED, &temp_conf_gint);

	while(temp_conf_gslist)
	{
		temp_conf_string = (gchar *) temp_conf_gslist->data;
		if (g_file_test(temp_conf_string, G_FILE_TEST_EXISTS))
		{
			cmsHPROFILE color_profile = cmsOpenProfileFromFile(temp_conf_string, "r");
			if (color_profile)
			{
				cmsCloseProfile(color_profile);	
				gtk_combo_box_append_text(GTK_COMBO_BOX(cms_ex_profile_combobox), g_basename(temp_conf_string));
				temp_new_gslist = g_slist_append(temp_new_gslist, (gpointer *) temp_conf_string);
			}
		}
		else
			temp_conf_gint = 0;
		temp_conf_gslist = temp_conf_gslist->next;
	}

	rs_conf_set_list_string(CONF_CMS_EX_PROFILE_LIST, temp_new_gslist);
	g_slist_free(temp_new_gslist);
	if (temp_conf_gint < 0)
		temp_conf_gint = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(cms_ex_profile_combobox), temp_conf_gint);
	cms_ex_profile_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start (GTK_BOX (cms_ex_profile_hbox), cms_ex_profile_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_ex_profile_hbox), cms_ex_profile_combobox, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_ex_profile_hbox), cms_ex_profile_button, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_page), cms_ex_profile_hbox, FALSE, TRUE, 0);

	cms_intent_hbox = gtk_hbox_new(FALSE, 0);
	cms_intent_label = gtk_label_new(_("Intent"));
	gtk_misc_set_alignment(GTK_MISC(cms_intent_label), 0.0, 0.5);
	cms_intent_combobox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_intent_combobox), _("Perceptual"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_intent_combobox), _("Relative colormetric"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_intent_combobox), _("Saturation"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(cms_intent_combobox), _("Absolute colormetric"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cms_intent_combobox), rs->cms_intent);
	gtk_box_pack_start (GTK_BOX (cms_intent_hbox), cms_intent_label, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_intent_hbox), cms_intent_combobox, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (cms_page), cms_intent_hbox, FALSE, TRUE, 0);

	g_signal_connect ((gpointer) cms_in_profile_combobox, "changed",
			G_CALLBACK(gui_cms_in_profile_combobox_changed), rs);
	g_signal_connect ((gpointer) cms_di_profile_combobox, "changed",
			G_CALLBACK(gui_cms_di_profile_combobox_changed), rs);
	g_signal_connect ((gpointer) cms_ex_profile_combobox, "changed",
			G_CALLBACK(gui_cms_ex_profile_combobox_changed), rs);
			
	g_signal_connect ((gpointer) cms_intent_combobox, "changed",
			G_CALLBACK(gui_cms_intent_combobox_changed), rs);
			
	g_signal_connect ((gpointer) cms_in_profile_button, "clicked",
			G_CALLBACK(gui_cms_in_profile_button_clicked), (gpointer) cms_in_profile_combobox);
	g_signal_connect ((gpointer) cms_di_profile_button, "clicked",
			G_CALLBACK(gui_cms_di_profile_button_clicked), (gpointer) cms_di_profile_combobox);
	g_signal_connect ((gpointer) cms_ex_profile_button, "clicked",
			G_CALLBACK(gui_cms_ex_profile_button_clicked), (gpointer) cms_ex_profile_combobox);

	return cms_page;
}
