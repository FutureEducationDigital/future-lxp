module ActiveAdmin
  module FoundersHelper
    def startup_token_collection
      (Founder.distinct.pluck(:startup_token) - [nil]).map do |token|
        ["Team lead by #{Founder.lead_of(token).email} (Invited on #{token})", token]
      end
    end
  end
end
