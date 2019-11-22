module Types
  class CourseType < Types::BaseObject
    field :id, ID, null: false
    field :name, String, null: false
    field :description, String, null: false
    field :ends_at, GraphQL::Types::ISO8601DateTime, null: true
    field :max_grade, Integer, null: false
    field :pass_grade, Integer, null: false
    field :grades_and_labels, [Types::GradeAndLabelType], null: false
    field :enable_leaderboard, Boolean, null: false
    field :about, String, null: true
    field :public_signup, Boolean, null: false
    field :image, Types::ImageType, null: true

    def grades_and_labels
      object.grade_labels.map do |grade, label|
        { grade: grade.to_i, label: label }
      end
    end

    def image
      object.image.attached? ? image_details(object.image) : nil
    end

    private

    def image_details(image)
      {
        url: Rails.application.routes.url_helpers.rails_blob_path(image, only_path: true),
        filename: image.filename
      }
    end
  end
end
