<?php


abstract class defaultBaseMigration extends Migration {

  /**
   * @var array $migrationData
   *  Raw array of migration files loaded from the yaml files.
   */
  protected $migrationData = array();
  /**
   * @var array $arguments
   *  Arguments passed when registering the migration.
   *  @see migrate_default_content_flush_caches()
   */
  protected $arguments;

  protected $mappedFields;

  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->migrationData = $this->getMigrationConfigData($arguments['machine_name']);;
    // Handle dependencies.
    if (!empty($this->migrationData['dependencies'])) {
      $this->dependencies = $this->migrationData['dependencies'];
    }
  }

  /**
   * Returns the corresponding migrate information among the yaml files in the
   * folder.
   *
   * @param string $machine_name
   *  The migration machine name defined in the file
   *
   * @return array
   */
  protected function getMigrationConfigData($machine_name) {
    $config = migrate_default_content_get_config_files();
    foreach ($config as $data) {
      if ($data['name'] == $machine_name) {
        return $data;
      }
    }
  }

  /**
   * Generate migration machine name from the yaml file machine name.
   * We need to override this method so the class name is not used.
   * @see https://drupal.org/node/1006998
   */
  protected function generateMachineName($class_name = NULL) {
    return $this->arguments['machine_name'];
  }

  protected function getMigrationSource() {
    $this->csvColumns = !empty($this->csvColumns) ? $this->csvColumns : array();
    $csv_cols[] = array('id', 'Unique ID');

    foreach ($this->migrationData['fields'] as $field) {
      $key = key($field);
      $value = $field[$key];
      if (!empty($value['source'])) {
        $csv_cols[] = array($key, $value['source']);
      }
    }

    // Rebuild the csv columns array.
    $this->csvColumns = array_merge($csv_cols, $this->csvColumns);

    // Merge default options with other specified in config files.
    $options = array('header_rows' => 1);
    if (array_key_exists('options', $this->migrationData)) {
      $options = array_merge($options, $this->migrationData['options'][0]);
    }
    return new MigrateSourceCSV($this->arguments['source_path'] . '/' . $this->arguments['source_file'], $this->csvColumns, $options);

  }

  protected function addDefaultMappings() {
    foreach ($this->migrationData['fields'] as $field) {
      $key = key($field);
      $value = $field[$key];
      if (is_array($value) && count($value) == 1) {
        $this->addFieldMapping($key, $key, FALSE);
      }
      else {
        $this->preprocessMapping($key, $value);
      }
    }
  }

  protected function preprocessMapping($key, $value) {
    if (!is_array($value)) {
      return;
    }

    // Handle default mapping, if there's any.
    if (!empty($value['source'])) {
      $this->addFieldMapping($key, $key, FALSE);
    }

    // Handle empty, default values.
    if (!empty($value['default_value'])) {
      $migrate = $this->addFieldMapping($key);
      $migrate->defaultValue($value['default_value']);
    }

    // Split values.
    if (!empty($value['separator'])) {
      $migrate = $this->addFieldMapping($key, $key, FALSE);
      $migrate->separator($value['separator']);
    }

    if (!empty($value['source_migration'])) {
      if (empty($migrate) || !$migrate instanceof MigrateFieldMapping) {
        $migrate = $this->addFieldMapping($key, $key, FALSE);
      }
      $separator = !empty($value['separator']) ? $value['separator'] : '|';
      $source_migration = explode($separator, $value['source_migration']);
      $migrate->sourceMigration($source_migration);
    }

    if (!empty($value['components']) && is_array($value['components'])) {
      foreach ($value['components'] as $name => $component) {
        if ($name == 'source_dir') {
          $component = $this->arguments['source_path'] . '/' . $component;
        }
        $migrate = $this->addFieldMapping($key . ':' . $name);
        $migrate->defaultValue($component);
      }
    }
  }

}
